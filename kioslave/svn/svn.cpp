/* This file is part of the KDE project
   Copyright (C) 2003 Mickael Marchand <marchand@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#define __STDC_CONSTANT_MACROS
#include <stdint.h>
#include <QDateTime>
#include <QBitArray>
#include <QTextStream>

#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <kdebug.h>
#include <kmessagebox.h>
#include <kcomponentdata.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kurl.h>
#include <ksvndinterface.h>
#include <subversion-1/svn_sorts.h>
#include <subversion-1/svn_path.h>
#include <subversion-1/svn_utf.h>
#include <subversion-1/svn_ra.h>
#include <subversion-1/svn_time.h>
#include <subversion-1/svn_fs.h>

#include <kmimetype.h>
#include <QFile>
#include <kde_file.h>
#include <time.h>
#include <utime.h>
 
#include "svn.h"
#include <apr_portable.h>

using namespace KIO;

typedef struct
{
	/* Holds the directory that corresponds to the REPOS_URL at RA->open()
	 *      time. When callbacks specify a relative path, they are joined with
	 *           this base directory. */
	const char *base_dir;
	svn_wc_adm_access_t *base_access;

	/* An array of svn_client_commit_item_t * structures, present only
	 *      during working copy commits. */
	apr_array_header_t *commit_items;

	/* A hash of svn_config_t's, keyed off file name (i.e. the contents of
	 *      ~/.subversion/config end up keyed off of 'config'). */
	apr_hash_t *config;

	/* The pool to use for session-related items. */
	apr_pool_t *pool;

} svn_client__callback_baton_t;

static svn_error_t *
open_tmp_file (apr_file_t **fp,
               void *callback_baton,
               apr_pool_t *pool)
{
  svn_client__callback_baton_t *cb = (svn_client__callback_baton_t *) callback_baton;
  const char *truepath;
  const char *ignored_filename;

  if (cb->base_dir)
    truepath = apr_pstrdup (pool, cb->base_dir);
  else
    truepath = "";

  /* Tack on a made-up filename. */
  truepath = svn_path_join (truepath, "tempfile", pool);

  /* Open a unique file;  use APR_DELONCLOSE. */
  SVN_ERR (svn_io_open_unique_file (fp, &ignored_filename,
                                    truepath, ".tmp", true, pool));

  return SVN_NO_ERROR;
}

static svn_error_t *write_to_string(void *baton, const char *data, apr_size_t *len) {
	kbaton *tb = ( kbaton* )baton;
	svn_stringbuf_appendbytes(tb->target_string, data, *len);
	return SVN_NO_ERROR;
}

static int
compare_items_as_paths (const svn_sort__item_t*a, const svn_sort__item_t*b) {
  return svn_path_compare_paths ((const char *)a->key, (const char *)b->key);
}

kio_svnProtocol::kio_svnProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
	: SlaveBase("kio_svn", pool_socket, app_socket) {
		kDebug(7128) << "kio_svnProtocol::kio_svnProtocol()";

		m_counter = 0;

		apr_initialize();
		pool = svn_pool_create (NULL);
		svn_error_t *err = svn_client_create_context(&ctx, pool);
		if ( err ) {
			kDebug(7128) << "kio_svnProtocol::kio_svnProtocol() create_context ERROR";
			error( KIO::ERR_SLAVE_DEFINED, err->message );
			return;
		}

		err = svn_config_ensure (NULL,pool);
		if ( err ) {
			kDebug(7128) << "kio_svnProtocol::kio_svnProtocol() configensure ERROR";
			error( KIO::ERR_SLAVE_DEFINED, err->message );
			return;
		}
		svn_config_get_config (&ctx->config,NULL,pool);

		ctx->log_msg_func = kio_svnProtocol::commitLogPrompt;
		ctx->log_msg_baton = this; //pass this so that we can get a dcopClient from it
		//TODO
		ctx->cancel_func = NULL;

		apr_array_header_t *providers = apr_array_make(pool, 9, sizeof(svn_auth_provider_object_t *));

		svn_auth_provider_object_t *provider;

		//disk cache
		svn_client_get_simple_provider(&provider,pool);
		APR_ARRAY_PUSH(providers, svn_auth_provider_object_t*) = provider;
		svn_client_get_username_provider(&provider,pool);
		APR_ARRAY_PUSH(providers, svn_auth_provider_object_t*) = provider;

		//interactive prompt
		svn_client_get_simple_prompt_provider (&provider,kio_svnProtocol::checkAuth,this,2,pool);
		APR_ARRAY_PUSH(providers, svn_auth_provider_object_t*) = provider;
		//we always ask user+pass, no need for a user only question
/*		svn_client_get_username_prompt_provider
 *		(&provider,kio_svnProtocol::checkAuth,this,2,pool);
		APR_ARRAY_PUSH(providers, svn_auth_provider_object_t*) = provider;*/

		//SSL disk cache, keep that one, because it does nothing bad :)
		svn_client_get_ssl_server_trust_file_provider (&provider, pool);
		APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
		svn_client_get_ssl_client_cert_file_provider (&provider, pool);
		APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
		svn_client_get_ssl_client_cert_pw_file_provider (&provider, pool);
		APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

		//SSL interactive prompt, where things get hard
		svn_client_get_ssl_server_trust_prompt_provider (&provider, kio_svnProtocol::trustSSLPrompt, NULL, pool);
		APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
		svn_client_get_ssl_client_cert_prompt_provider (&provider, kio_svnProtocol::clientCertSSLPrompt, NULL, 2, pool);
		APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;
		svn_client_get_ssl_client_cert_pw_prompt_provider (&provider, kio_svnProtocol::clientCertPasswdPrompt, NULL, 2, pool);
		APR_ARRAY_PUSH (providers, svn_auth_provider_object_t *) = provider;

		svn_auth_open(&ctx->auth_baton, providers, pool);
}

kio_svnProtocol::~kio_svnProtocol(){
	kDebug(7128) << "kio_svnProtocol::~kio_svnProtocol()";
	svn_pool_destroy(pool);
	apr_terminate();
}

void kio_svnProtocol::initNotifier(bool is_checkout, bool is_export, bool suppress_final_line, apr_pool_t *spool) {
	m_counter=0;//reset counter
	ctx->notify_func = kio_svnProtocol::notify;
	struct notify_baton *nb = ( struct notify_baton* )apr_palloc(spool, sizeof( *nb ) );
	nb->master = this;
	nb->received_some_change = false;
	nb->sent_first_txdelta = false;
	nb->is_checkout = is_checkout;
	nb->is_export = is_export;
	nb->suppress_final_line = suppress_final_line;
	nb->in_external = false;
	nb->had_print_error = false;
	nb->pool = svn_pool_create (spool);

	ctx->notify_baton = nb;
}

svn_error_t* kio_svnProtocol::checkAuth(svn_auth_cred_simple_t **cred, void *baton, const char *realm, const char *username, svn_boolean_t /*may_save*/, apr_pool_t *pool) {
	kDebug(7128) << "kio_svnProtocol::checkAuth() for " << realm;
	kio_svnProtocol *p = ( kio_svnProtocol* )baton;
	svn_auth_cred_simple_t *ret = (svn_auth_cred_simple_t*)apr_pcalloc (pool, sizeof (*ret));

//	p->info.keepPassword = true;
	p->info.verifyPath=true;
	kDebug(7128 ) << "auth current URL : " << p->myURL.url();
	p->info.url = p->myURL;
	p->info.username = username; //( const char* )svn_auth_get_parameter( p->ctx->auth_baton, SVN_AUTH_PARAM_DEFAULT_USERNAME );
//	if ( !p->checkCachedAuthentication( p->info ) ){
		p->openPasswordDialog( p->info );
//	}
	ret->username = apr_pstrdup(pool, p->info.username.toUtf8());
	ret->password = apr_pstrdup(pool, p->info.password.toUtf8());
	ret->may_save = true;
	*cred = ret;
	return SVN_NO_ERROR;
}

void kio_svnProtocol::recordCurrentURL(const KUrl& url) {
	myURL = url;
}

//don't implement mimeType() until we don't need to download the whole file

void kio_svnProtocol::get(const KUrl& url ){
	kDebug(7128) << "kio_svn::get(const KUrl& url)";

	QString remoteServer = url.host();
	infoMessage(i18n("Looking for %1...", remoteServer ) );

	apr_pool_t *subpool = svn_pool_create (pool);
	kbaton *bt = (kbaton*)apr_pcalloc(subpool, sizeof(*bt));
	bt->target_string = svn_stringbuf_create("", subpool);
	bt->string_stream = svn_stream_create(bt,subpool);
	svn_stream_set_write(bt->string_stream,write_to_string);

	QString target = makeSvnURL( url );
	kDebug(7128) << "SvnURL: " << target;
	recordCurrentURL( KUrl( target ) );

	//find the requested revision
	svn_opt_revision_t rev;
	svn_opt_revision_t endrev;
	int idx = target.lastIndexOf( "?rev=" );
	if ( idx != -1 ) {
		QString revstr = target.mid( idx+5 );
#if 0
		kDebug(7128) << "revision string found " << revstr;
		if ( revstr == "HEAD" ) {
			rev.kind = svn_opt_revision_head;
			kDebug(7128) << "revision searched : HEAD";
		} else {
			rev.kind = svn_opt_revision_number;
			rev.value.number = revstr.toLong();
			kDebug(7128) << "revision searched : " << rev.value.number;
		}
#endif
		svn_opt_parse_revision( &rev, &endrev, revstr.toUtf8(), subpool );
		target = target.left( idx );
		kDebug(7128) << "new target : " << target;
	} else {
		kDebug(7128) << "no revision given. searching HEAD ";
		rev.kind = svn_opt_revision_head;
	}
	initNotifier(false, false, false, subpool);

	svn_error_t *err = svn_client_cat (bt->string_stream, svn_path_canonicalize( target.toUtf8(),subpool ),&rev,ctx, subpool);
	if ( err ) {
		error( KIO::ERR_SLAVE_DEFINED, err->message );
		svn_pool_destroy( subpool );
		return;
	}

	// Send the mimeType as soon as it is known
	QByteArray cp = QByteArray::fromRawData(bt->target_string->data, bt->target_string->len);
	KMimeType::Ptr mt = KMimeType::findByNameAndContent(url.fileName(), cp);
	kDebug(7128) << "KMimeType returned : " << mt->name();
	mimeType( mt->name() );

	totalSize(bt->target_string->len);

	//send data
	data(cp);

	data(QByteArray()); // empty array means we're done sending the data
	finished();
	svn_pool_destroy (subpool);
}

//this is PUT-ing to a -remote- repository for now
//we are using the "svnput" hack, that is we just overwrite what's in the repository
//THIS IS BAD, we have no safety check that nobody committed something else just before we do the put
//but there is no other way to do it for now, still better than nothing, but use with REAL CARE
void kio_svnProtocol::put(const KUrl& url, int /*permissions*/, KIO::JobFlags /*flags*/) {
	//putting things is actually just commiting to a repository, so let's do it
	kDebug(7128) << "kio_svn::put : " << url.url();
	QByteArray ba;
	QTemporaryFile tFile;
	int result=0;

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_error_t *err;
	svn_ra_callbacks_t *cbtable;
	const char *parent_URL, *basename;
	svn_ra_plugin_t *ra_lib;
	void *session,*ra_baton;
	svn_revnum_t rev;
	apr_hash_t *dirents;
	svn_dirent_t *dirent;
	void *root_baton, *file_baton, *handler_baton;
	svn_txdelta_window_handler_t handler;
	svn_stream_t *contents;
	apr_file_t *f = NULL;
	const QString mtimeStr = metaData( "modified" );

	err = svn_fs_initialize (subpool);
	if (err) goto handleerror;

	cbtable = (svn_ra_callbacks_t *)apr_pcalloc (subpool, sizeof(*cbtable));
	cbtable->auth_baton = ctx->auth_baton;
	cbtable->open_tmp_file = open_tmp_file;

	svn_path_split (url.url().toUtf8(), &parent_URL, &basename, subpool);

	err = svn_ra_init_ra_libs (&ra_baton, pool);
	if (err) goto handleerror;

	err = svn_ra_get_ra_library (&ra_lib, ra_baton, parent_URL, subpool);
	if (err) goto handleerror;
	err = ra_lib->open (&session, parent_URL, cbtable, NULL, ctx->config, subpool);
	if (err) goto handleerror;
	err = ra_lib->get_latest_revnum (session, &rev, subpool);
	if (err) goto handleerror;
	err = ra_lib->get_dir (session, "", rev, &dirents, NULL, NULL, subpool);
	if (err) goto handleerror;
	dirent = (svn_dirent_t *)apr_hash_get (dirents, basename, APR_HASH_KEY_STRING);
	if (dirent && dirent->kind == svn_node_dir) {
		kDebug(7128) << "Sorry, a directory already exists at that URL.";
		error( KIO::ERR_SLAVE_DEFINED, i18n("For reasons of safety, directories are not yet supported.") );
		svn_pool_destroy( subpool );
		return;
	}
/*	if (dirent && dirent->kind == svn_node_file)
	{
		//confirm XXX
	}*/
	const svn_delta_editor_t *editor;
	void *edit_baton;
	err = ra_lib->get_commit_editor (session, &editor, &edit_baton, "Automated commit from KDE KIO Subversion\n", NULL/*commitcb*/, NULL, subpool);
	if (err) goto handleerror;

	err = editor->open_root (edit_baton, rev, subpool, &root_baton);
	if (err) goto handleerror;
	if (! dirent) {
		err = editor->add_file (basename, root_baton, NULL, SVN_INVALID_REVNUM, subpool, &file_baton);
	} else {
		err = editor->open_file (basename, root_baton, rev, subpool, &file_baton);
	}
	if (err) goto handleerror;
	err = editor->apply_textdelta (file_baton, NULL, subpool, &handler, &handler_baton);
	if (err) goto handleerror;

	if (!tFile.open()) {
		kDebug(7128) << "Failed creating temp file";
		return;
	}

	do {
		dataReq();
		result = readData(ba);
		if ( result >= 0 ) {
			tFile.write(ba);
//			kDebug(7128) << "Sending bytes : " << result;
//			err = svn_txdelta_send_string ( svn_string_ncreate(ba.constData(),ba.size(),subpool), handler, handler_baton, subpool);
//			if (err) goto handleerror;
//			kDebug(7128) << "Done sending bytes";
		}
	} while (result > 0);
	tFile.flush();
	kDebug(7128) << "Temp file flushed to " << tFile.fileName();
	err = svn_io_file_open (&f, tFile.fileName().toUtf8(), APR_READ, APR_OS_DEFAULT, subpool);
	if (err) goto handleerror;
	contents = svn_stream_from_aprfile (f, pool);
	err = svn_txdelta_send_stream (contents, handler, handler_baton, NULL, subpool);
	if (err) goto handleerror;
	err = svn_io_file_close (f, subpool);
	if (err) goto handleerror;
	err = editor->close_file (file_baton, NULL, subpool);
	if (err) goto handleerror;
	err = editor->close_edit (edit_baton, subpool);
	if (err) goto handleerror;

	//all good	
    // set modification time
	// XXX wtf this is never called ...
	if ( !mtimeStr.isEmpty() ) {
		QDateTime dt = QDateTime::fromString( mtimeStr, Qt::ISODate );
		kDebug(7128) << "MOD TIME : " << dt ;
		if ( dt.isValid() ) {
			KDE_struct_stat dest_statbuf;
			kDebug(7128) << "KDE_stat : " << url;
			if (KDE_stat( url.url().toUtf8().constData(), &dest_statbuf ) == 0) {
				struct utimbuf utbuf;
				utbuf.actime = dest_statbuf.st_atime; // access time, unchanged
				utbuf.modtime = dt.toTime_t(); // modification time
				kDebug(7128) << "SHOULD update mtime remotely ? " << dt;
//				utime( url.url().toUtf8().constData(), &utbuf );
			}
		}
	}

	finished();
	return;

handleerror:
	error( KIO::ERR_SLAVE_DEFINED, err->message );
	svn_pool_destroy( subpool );
	return;
}

void kio_svnProtocol::stat(const KUrl & url) {
	kDebug(7128) << "kio_svn::stat(const KUrl& url) : " << url.url();

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_ra_session_t	*session;
	svn_error_t *err;
	UDSEntry entry;
	const char *author;

	QString target = makeSvnURL(url);
	kDebug(7128) << "SvnURL: " << target;
	recordCurrentURL( KUrl( target ) );

	//find the requested revision
	svn_opt_revision_t rev;
	svn_opt_revision_t endrev;
	int idx = target.lastIndexOf( "?rev=" );
	if ( idx != -1 ) {
		QString revstr = target.mid( idx+5 );
		svn_opt_parse_revision( &rev, &endrev, revstr.toUtf8( ), subpool );
		target = target.left( idx );
		kDebug(7128) << "new target : " << target;
	} else {
		kDebug(7128) << "no revision given. searching HEAD ";
		rev.kind = svn_opt_revision_head;
	}

	//init
	err = svn_ra_initialize(subpool);
	if ( err ) {
		kDebug(7128) << "init RA libs failed : " << err->message;
		error( KIO::ERR_SLAVE_DEFINED, err->message );
		svn_pool_destroy( subpool );
		return;
	}
	kDebug(7128) << "RA init completed";

	//start session
	svn_ra_callbacks_t *cbtable = (svn_ra_callbacks_t*)apr_pcalloc(subpool, sizeof(*cbtable));
	kio_svn_callback_baton_t *callbackbt = (kio_svn_callback_baton_t*)apr_pcalloc(subpool, sizeof( *callbackbt ));

	cbtable->open_tmp_file = open_tmp_file;
	cbtable->get_wc_prop = NULL;
	cbtable->set_wc_prop = NULL;
	cbtable->push_wc_prop = NULL;
	cbtable->auth_baton = ctx->auth_baton;

	callbackbt->base_dir = target.toUtf8();
	callbackbt->pool = subpool;
	callbackbt->config = ctx->config;

	err = svn_ra_open(&session,svn_path_canonicalize( target.toUtf8(), subpool ),cbtable,callbackbt,ctx->config,subpool);
	if ( err ) {
		kDebug(7128)<< "Open session " << err->message;
		error( KIO::ERR_SLAVE_DEFINED, err->message );
		svn_pool_destroy( subpool );
		return;
	}
	kDebug(7128) << "Session opened to " << target;
	//find number for HEAD
	if (rev.kind == svn_opt_revision_head) {
		err = svn_ra_get_latest_revnum(session, &rev.value.number, subpool);
		if ( err ) {
			kDebug(7128)<< "Latest RevNum " << err->message;
			error( KIO::ERR_SLAVE_DEFINED, err->message );
			svn_pool_destroy( subpool );
			return;
		}
		kDebug(7128) << "Got rev " << rev.value.number;
	}

	//get it
	svn_dirent_t *dirent;
	err = svn_ra_stat (session, "", rev.value.number, &dirent, subpool);
	if ( err ) {
			kDebug(7128)<< "RA Stat " << err->message;
			error( KIO::ERR_SLAVE_DEFINED, err->message );
			svn_pool_destroy( subpool );
			return;
	}
	kDebug(7128) << "Checked Path" << svn_path_canonicalize( target.toUtf8(), subpool );
	if (dirent) {
		svn_utf_cstring_from_utf8 (&author, dirent->last_author, subpool);
		if (dirent->kind==svn_node_dir || dirent->kind==svn_node_file) {
			kDebug(7128) << "Creating UDSEntry " << url.fileName();
			createUDSEntry(url.fileName(),author,dirent->size,dirent->kind==svn_node_dir?true:false, apr_time_sec(dirent->time),entry);
			statEntry( entry );
		}
	}
	finished();
	svn_pool_destroy( subpool );
}

void kio_svnProtocol::listDir(const KUrl& url) {
	kDebug(7128) << "kio_svn::listDir(const KUrl& url) : " << url.url();

	apr_pool_t *subpool = svn_pool_create (pool);
	apr_hash_t *dirents;

	QString target = makeSvnURL( url);
	kDebug(7128) << "SvnURL: " << target;
	recordCurrentURL( KUrl( target ) );

	//find the requested revision
	svn_opt_revision_t rev;
	svn_opt_revision_t endrev;
	int idx = target.lastIndexOf( "?rev=" );
	if ( idx != -1 ) {
		QString revstr = target.mid( idx+5 );
		svn_opt_parse_revision( &rev, &endrev, revstr.toUtf8(), subpool );
		target = target.left( idx );
		kDebug(7128) << "new target : " << target;
	} else {
		kDebug(7128) << "no revision given. searching HEAD ";
		rev.kind = svn_opt_revision_head;
	}

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_ls (&dirents, svn_path_canonicalize( target.toUtf8(), subpool ), &rev, false, ctx, subpool);
	if ( err ) {
		error( KIO::ERR_SLAVE_DEFINED, err->message );
		svn_pool_destroy( subpool );
		return;
	}

  apr_array_header_t *array;
  int i;

  array = svn_sort__hash (dirents, compare_items_as_paths, subpool);

  UDSEntry entry;
  for (i = 0; i < array->nelts; ++i) {
	  entry.clear();
	  const char *utf8_entryname, *native_entryname;
	  svn_dirent_t *dirent;
	  svn_sort__item_t *item;

	  item = &APR_ARRAY_IDX (array, i, svn_sort__item_t);

	  utf8_entryname = (const char*)item->key;

	  dirent = (svn_dirent_t*)apr_hash_get (dirents, utf8_entryname, item->klen);

	  svn_utf_cstring_from_utf8 (&native_entryname, utf8_entryname, subpool);
	  const char *native_author = NULL;
	  time_t mtime = apr_time_sec(dirent->time);
	  if (dirent->last_author)
		  svn_utf_cstring_from_utf8 (&native_author, dirent->last_author, subpool);

	  if (dirent->last_author)
		  svn_utf_cstring_from_utf8 (&native_author, dirent->last_author, subpool);

	  if ( createUDSEntry(QString( native_entryname ), QString( native_author ), dirent->size,
				  dirent->kind==svn_node_dir ? true : false, mtime, entry) )
		  listEntry( entry, false );
  }
	listEntry( entry, true );

	finished();
	svn_pool_destroy (subpool);
}

bool kio_svnProtocol::createUDSEntry( const QString& filename, const QString& user, long long int size, bool isdir, time_t mtime, UDSEntry& entry) {
	kDebug(7128) << "MTime : " << ( long )mtime;
	kDebug(7128) << "UDS filename : " << filename;

	mode_t access;
	mode_t type = isdir?S_IFDIR:S_IFREG;
	access = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
	
	entry.insert(KIO::UDSEntry::UDS_NAME,filename);
	entry.insert(KIO::UDSEntry::UDS_ACCESS,access);
	entry.insert(KIO::UDSEntry::UDS_FILE_TYPE,type);
	entry.insert(KIO::UDSEntry::UDS_SIZE,size);
	entry.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME,mtime);
	entry.insert(KIO::UDSEntry::UDS_USER,user);

	return true;
}

void kio_svnProtocol::copy(const KUrl & src, const KUrl& dest, int /*permissions*/, KIO::JobFlags) {
	kDebug(7128) << "kio_svnProtocol::copy() Source : " << src.url() << " Dest : " << dest.url();

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_client_commit_info_t *commit_info = NULL;

	KUrl nsrc = src;
	KUrl ndest = dest;
	nsrc.setProtocol( chooseProtocol( src.protocol() ) );
	ndest.setProtocol( chooseProtocol( dest.protocol() ) );
	QString srcsvn = nsrc.url();
	QString destsvn = ndest.url();

	recordCurrentURL( nsrc );

	//find the requested revision
	svn_opt_revision_t rev;
	int idx = srcsvn.lastIndexOf( "?rev=" );
	if ( idx != -1 ) {
		QString revstr = srcsvn.mid( idx+5 );
		kDebug(7128) << "revision string found " << revstr;
		if ( revstr == "HEAD" ) {
			rev.kind = svn_opt_revision_head;
			kDebug(7128) << "revision searched : HEAD";
		} else {
			rev.kind = svn_opt_revision_number;
			rev.value.number = revstr.toLong();
			kDebug(7128) << "revision searched : " << rev.value.number;
		}
		srcsvn = srcsvn.left( idx );
		kDebug(7128) << "new src : " << srcsvn;
	} else {
		kDebug(7128) << "no revision given. searching HEAD ";
		rev.kind = svn_opt_revision_head;
	}

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_copy(&commit_info, srcsvn.toUtf8(), &rev, destsvn.toUtf8(), ctx, subpool);
	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::mkdir( const KUrl::List& list, int /*permissions*/ ) {
	kDebug(7128) << "kio_svnProtocol::mkdir(LIST) : " << list;

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_client_commit_info_t *commit_info = NULL;

	recordCurrentURL( list[ 0 ] );

	apr_array_header_t *targets = apr_array_make(subpool, list.count()+1, sizeof(const char *));

	KUrl::List::const_iterator it = list.begin(), end = list.end();
	for ( ; it != end; ++it ) {
		QString cur = makeSvnURL( *it );
		kDebug( 7128 ) << "kio_svnProtocol::mkdir raw url for subversion : " << cur;
		const char *_target = apr_pstrdup( subpool, svn_path_canonicalize( apr_pstrdup( subpool, cur.toUtf8() ), subpool ) );
		(*(( const char ** )apr_array_push(( apr_array_header_t* )targets)) ) = _target;
	}

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_mkdir(&commit_info,targets,ctx,subpool);
	if ( err )
		error( KIO::ERR_COULD_NOT_MKDIR, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::mkdir( const KUrl& url, int /*permissions*/ ) {
	kDebug(7128) << "kio_svnProtocol::mkdir() : " << url.url();

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_client_commit_info_t *commit_info = NULL;

	QString target = makeSvnURL( url);
	kDebug(7128) << "SvnURL: " << target;
	recordCurrentURL( KUrl( target ) );

	apr_array_header_t *targets = apr_array_make(subpool, 2, sizeof(const char *));
	(*(( const char ** )apr_array_push(( apr_array_header_t* )targets)) ) = apr_pstrdup( subpool, target.toUtf8() );

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_mkdir(&commit_info,targets,ctx,subpool);
	if ( err )
		error( KIO::ERR_COULD_NOT_MKDIR, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::del( const KUrl& url, bool /*isfile*/ ) {
	kDebug(7128) << "kio_svnProtocol::del() : " << url.url();

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_client_commit_info_t *commit_info = NULL;

	QString target = makeSvnURL(url);
	kDebug(7128) << "SvnURL: " << target;
	recordCurrentURL( KUrl( target ) );

	apr_array_header_t *targets = apr_array_make(subpool, 2, sizeof(const char *));
	(*(( const char ** )apr_array_push(( apr_array_header_t* )targets)) ) = apr_pstrdup( subpool, target.toUtf8() );

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_delete(&commit_info,targets,false/*force remove locally modified files in wc*/,ctx,subpool);
	if ( err )
		error( KIO::ERR_CANNOT_DELETE, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::rename(const KUrl& src, const KUrl& dest, KIO::JobFlags) {
	kDebug(7128) << "kio_svnProtocol::rename() Source : " << src.url() << " Dest : " << dest.url();

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_client_commit_info_t *commit_info = NULL;

	KUrl nsrc = src;
	KUrl ndest = dest;
	nsrc.setProtocol( chooseProtocol( src.protocol() ) );
	ndest.setProtocol( chooseProtocol( dest.protocol() ) );
	QString srcsvn = nsrc.url();
	QString destsvn = ndest.url();

	recordCurrentURL( nsrc );

	//find the requested revision
	svn_opt_revision_t rev;
	int idx = srcsvn.lastIndexOf( "?rev=" );
	if ( idx != -1 ) {
		QString revstr = srcsvn.mid( idx+5 );
		kDebug(7128) << "revision string found " << revstr;
		if ( revstr == "HEAD" ) {
			rev.kind = svn_opt_revision_head;
			kDebug(7128) << "revision searched : HEAD";
		} else {
			rev.kind = svn_opt_revision_number;
			rev.value.number = revstr.toLong();
			kDebug(7128) << "revision searched : " << rev.value.number;
		}
		srcsvn = srcsvn.left( idx );
		kDebug(7128) << "new src : " << srcsvn;
	} else {
		kDebug(7128) << "no revision given. searching HEAD ";
		rev.kind = svn_opt_revision_head;
	}

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_move(&commit_info, srcsvn.toUtf8(), &rev, destsvn.toUtf8(), false/*force remove locally modified files in wc*/, ctx, subpool);
	if ( err )
		error( KIO::ERR_CANNOT_RENAME, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::special( const QByteArray& data ) {
	kDebug(7128) << "kio_svnProtocol::special";

	QByteArray tmpData(data);
	QDataStream stream(&tmpData, QIODevice::ReadOnly);
	int tmp;

	stream >> tmp;
	kDebug(7128) << "kio_svnProtocol::special " << tmp;

	switch ( tmp ) {
		case SVN_CHECKOUT:
			{
				KUrl repository, wc;
				int revnumber;
				QString revkind;
				stream >> repository;
				stream >> wc;
				stream >> revnumber;
				stream >> revkind;
				kDebug(7128) << "kio_svnProtocol CHECKOUT from " << repository.url() << " to " << wc.url() << " at " << revnumber << " or " << revkind;
				checkout( repository, wc, revnumber, revkind );
				break;
			}
		case SVN_UPDATE:
			{
				KUrl wc;
				int revnumber;
				QString revkind;
				stream >> wc;
				stream >> revnumber;
				stream >> revkind;
				kDebug(7128) << "kio_svnProtocol UPDATE " << wc.url() << " at " << revnumber << " or " << revkind;
				update(wc, revnumber, revkind );
				break;
			}
		case SVN_COMMIT:
			{
				KUrl::List wclist;
				while ( !stream.atEnd() ) {
					KUrl tmp;
					stream >> tmp;
					wclist << tmp;
				}
				kDebug(7128) << "kio_svnProtocol COMMIT";
				commit( wclist );
				break;
			}
		case SVN_LOG:
			{
				kDebug(7128) << "kio_svnProtocol LOG";
				int revstart, revend;
				QString revkindstart, revkindend;
				KUrl::List targets;
				stream >> revstart;
				stream >> revkindstart;
				stream >> revend;
				stream >> revkindend;
				while ( !stream.atEnd() ) {
					KUrl tmp;
					stream >> tmp;
					targets << tmp;
				}
				svn_log( revstart, revkindstart, revend, revkindend, targets );
				break;
			}
		case SVN_IMPORT:
			{
				KUrl wc,repos;
				stream >> repos;
				stream >> wc;
				kDebug(7128) << "kio_svnProtocol IMPORT";
				import(repos,wc);
				break;
			}
		case SVN_ADD:
			{
				KUrl wc;
				stream >> wc;
				kDebug(7128) << "kio_svnProtocol ADD";
				add(wc);
				break;
			}
		case SVN_DEL:
			{
				KUrl::List wclist;
				while ( !stream.atEnd() ) {
					KUrl tmp;
					stream >> tmp;
					wclist << tmp;
				}
				kDebug(7128) << "kio_svnProtocol DEL";
				wc_delete(wclist);
				break;
			}
		case SVN_REVERT:
			{
				KUrl::List wclist;
				while ( !stream.atEnd() ) {
					KUrl tmp;
					stream >> tmp;
					wclist << tmp;
				}
				kDebug(7128) << "kio_svnProtocol REVERT";
				wc_revert(wclist);
				break;
			}
		case SVN_STATUS:
			{
				KUrl wc;
				int checkRepos=false;
				int fullRecurse=false;
				stream >> wc;
				stream >> checkRepos;
				stream >> fullRecurse;
				kDebug(7128) << "kio_svnProtocol STATUS";
				wc_status(wc,checkRepos,fullRecurse);
				break;
			}
		case SVN_MKDIR:
			{
				KUrl::List list;
				stream >> list;
				kDebug(7128) << "kio_svnProtocol MKDIR";
				mkdir(list,0);
				break;
			}
		case SVN_RESOLVE:
			{
				KUrl url;
				bool recurse;
				stream >> url;
				stream >> recurse;
				kDebug(7128) << "kio_svnProtocol RESOLVE";
				wc_resolve(url,recurse);
				break;
			}
		case SVN_SWITCH:
			{
				KUrl wc,url;
				bool recurse;
				int revnumber;
				QString revkind;
				stream >> wc;
				stream >> url;
				stream >> recurse;
				stream >> revnumber;
				stream >> revkind;
				kDebug(7128) << "kio_svnProtocol SWITCH";
				svn_switch(wc,url,revnumber,revkind,recurse);
				break;
			}
		case SVN_DIFF:
			{
				KUrl url1,url2;
				int rev1, rev2;
				bool recurse;
				QString revkind1, revkind2;
				stream >> url1;
				stream >> url2;
				stream >> rev1;
				stream >> revkind1;
				stream >> rev2;
				stream >> revkind2;
				stream >> recurse;
				kDebug(7128) << "kio_svnProtocol DIFF";
				svn_diff(url1,url2,rev1,rev2,revkind1,revkind2,recurse);
				break;
			}
		default:
			{
				kDebug(7128) << "kio_svnProtocol DEFAULT";
				break;
			}
	}
}

void kio_svnProtocol::popupMessage( const QString& message ) {
	OrgKdeKsvndInterface ksvndInterface( "org.kde.kded", "/modules/ksvnd", QDBusConnection::sessionBus() );
	if(!ksvndInterface.isValid()) {
	   kWarning() << "Communication with KDED:KSvnd failed";
	   return;
	}
	QDBusReply<void> reply = ksvndInterface.popupMessage(message);
	if ( !reply.isValid() ) {
		kWarning() << "Unexpected reply type";
		return;
	}
}

void kio_svnProtocol::svn_log( int revstart, const QString& revkindstart, int revend, const QString& revkindend, const KUrl::List& targets ) {
	kDebug(7128) << "kio_svn::log : " << targets << " from revision " << revstart << " or " << revkindstart << " to "
		" revision " << revend << " or " << revkindend
		<< endl;

	apr_pool_t *subpool = svn_pool_create (pool);

	svn_opt_revision_t rev1 = createRevision( revstart, revkindstart, subpool );
	svn_opt_revision_t rev2 = createRevision( revend, revkindend, subpool );

	//TODO

	finished();
	svn_pool_destroy (subpool);
}

svn_opt_revision_t kio_svnProtocol::createRevision( int revision, const QString& revkind, apr_pool_t *pool ) {
	svn_opt_revision_t result,endrev;

	if ( revision != -1 ) {
		result.value.number = revision;
		result.kind = svn_opt_revision_number;
	} else if ( revkind == "WORKING" ) {
		result.kind = svn_opt_revision_working;
	} else if ( revkind == "BASE" ) {
		result.kind = svn_opt_revision_base;
	} else if ( revkind == "HEAD" ) {
        result.kind = svn_opt_revision_head;
    } else if ( revkind == "COMMITTED" ) {
        result.kind = svn_opt_revision_committed;
    } else if ( revkind == "PREV" ) {
        result.kind = svn_opt_revision_previous;
    }

    else if ( !revkind.isNull() ) {
		svn_opt_parse_revision(&result,&endrev,revkind.toUtf8(),pool);
	}else {
        result.kind = svn_opt_revision_unspecified;
    }
	return result;
}

void kio_svnProtocol::svn_diff(const KUrl & url1, const KUrl& url2,int rev1, int rev2,const QString& revkind1,const QString& revkind2,bool recurse) {
	kDebug(7128) << "kio_svn::diff : " << url1.path() << " at revision " << rev1 << " or " << revkind1 << " with "
		<< url2.path() << " at revision " << rev2 << " or " << revkind2
		<< endl ;

	apr_pool_t *subpool = svn_pool_create (pool);
	apr_array_header_t *options = svn_cstring_split( "", "\t\r\n", true, subpool );

	KUrl nurl1 = url1;
	KUrl nurl2 = url2;
	nurl1.setProtocol( chooseProtocol( url1.protocol() ) ); //svn+https -> https for eg
	nurl2.setProtocol( chooseProtocol( url2.protocol() ) );
	recordCurrentURL( nurl1 );
	QString source = makeSvnURL( nurl1 );
	QString target = makeSvnURL( nurl2 );

	const char *path1 = svn_path_canonicalize( apr_pstrdup( subpool, source.toUtf8() ), subpool );
	const char *path2 = svn_path_canonicalize( apr_pstrdup( subpool, target.toUtf8() ), subpool );
	//remove file:/// so we can diff for working copies, needs a better check (so we support URL for file:/// _repositories_ )
	if ( nurl1.protocol() == "file" ) {
		path1 = svn_path_canonicalize( apr_pstrdup( subpool, nurl1.path().toUtf8() ), subpool );
	}
	if ( nurl2.protocol() == "file" ) {
		path2 = svn_path_canonicalize( apr_pstrdup( subpool, nurl2.path().toUtf8() ), subpool );
	}
	kDebug( 7128 ) << "1 : " << path1 << " 2: " << path2;

	svn_opt_revision_t revision1,revision2;
	revision1 = createRevision(rev1, revkind1, subpool);
	revision2 = createRevision(rev2, revkind2, subpool);

	char *templ;
    templ = apr_pstrdup ( subpool, "/tmp/tmpfile_XXXXXX" );
	apr_file_t *outfile = NULL;
	apr_file_mktemp( &outfile, templ , APR_READ|APR_WRITE|APR_CREATE|APR_TRUNCATE, subpool );

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_diff (options, path1, &revision1, path2, &revision2, recurse, false, true, outfile, NULL, ctx, subpool);
	if ( err ) {
		error( KIO::ERR_SLAVE_DEFINED, err->message );
		svn_pool_destroy (subpool);
		return;
	}
	//read the content of the outfile now
	QStringList tmp;
	apr_file_close(outfile);
	QFile file(templ);
	if ( file.open(  QIODevice::ReadOnly ) ) {
		QTextStream stream(  &file );
		QString line;
		while ( !stream.atEnd() ) {
			line = stream.readLine();
			tmp << line;
		}
		file.close();
	}
	for ( QStringList::const_iterator itt = tmp.constBegin(); itt != tmp.constEnd(); ++itt ) {
		setMetaData(QString::number( m_counter ).rightJustified( 10,'0' )+ "diffresult", ( *itt ) );
		m_counter++;
	}
	//delete temp file
	file.remove();

	finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::svn_switch( const KUrl& wc, const KUrl& repos, int revnumber, const QString& revkind, bool recurse) {
	kDebug(7128) << "kio_svn::switch : " << wc.path() << " at revision " << revnumber << " or " << revkind;

	apr_pool_t *subpool = svn_pool_create (pool);

	KUrl nurl = repos;
	KUrl dest = wc;
	nurl.setProtocol( chooseProtocol( repos.protocol() ) );
	dest.setProtocol( "file" );
	recordCurrentURL( nurl );
	QString source = dest.path();
	QString target = makeSvnURL( repos );

	const char *path = svn_path_canonicalize( apr_pstrdup( subpool, source.toUtf8() ), subpool );
	const char *url = svn_path_canonicalize( apr_pstrdup( subpool, target.toUtf8() ), subpool );

	svn_opt_revision_t rev = createRevision( revnumber, revkind, subpool );

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_switch (NULL/*result revision*/, path, url, &rev, recurse, ctx, subpool);
	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::update( const KUrl& wc, int revnumber, const QString& revkind ) {
	kDebug(7128) << "kio_svn::update : " << wc.path() << " at revision " << revnumber << " or " << revkind;

	apr_pool_t *subpool = svn_pool_create (pool);
	KUrl dest = wc;
	dest.setProtocol( "file" );
	QString target = dest.path();
	recordCurrentURL( dest );

	svn_opt_revision_t rev = createRevision( revnumber, revkind, subpool );

//	apr_array_header_t *targets = apr_array_make(subpool, 1, sizeof(const char *));
//	const char *_target = apr_pstrdup( subpool, svn_path_canonicalize( target.toUtf8() , subpool ) );
//	(*(( const char ** )apr_array_push(( apr_array_header_t* )targets)) ) = _target;

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_update (NULL, svn_path_canonicalize( target.toUtf8(), subpool ), &rev, true, ctx, subpool);
//	svn_error_t *err = svn_client_update2 (NULL, targets, &rev, true, false, ctx, subpool);
	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::import( const KUrl& repos, const KUrl& wc ) {
	kDebug(7128) << "kio_svnProtocol::import() : " << wc.url() << " into " << repos.url();

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_client_commit_info_t *commit_info = NULL;
	bool nonrecursive = false;

	KUrl nurl = repos;
	KUrl dest = wc;
	nurl.setProtocol( chooseProtocol( repos.protocol() ) );
	dest.setProtocol( "file" );
	recordCurrentURL( nurl );
	dest.cleanPath( KUrl::SimplifyDirSeparators ); // remove doubled '/'
	QString source = dest.path( KUrl::RemoveTrailingSlash );
	QString target = makeSvnURL( repos );

	const char *path = svn_path_canonicalize( apr_pstrdup( subpool, source.toUtf8() ), subpool );
	const char *url = svn_path_canonicalize( apr_pstrdup( subpool, target.toUtf8() ), subpool );

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_import(&commit_info,path,url,nonrecursive,ctx,subpool);
	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::checkout( const KUrl& repos, const KUrl& wc, int revnumber, const QString& revkind ) {
	kDebug(7128) << "kio_svn::checkout : " << repos.url() << " into " << wc.path() << " at revision " << revnumber << " or " << revkind;

	apr_pool_t *subpool = svn_pool_create (pool);
	KUrl nurl = repos;
	KUrl dest = wc;
	nurl.setProtocol( chooseProtocol( repos.protocol() ) );
	dest.setProtocol( "file" );
	QString target = makeSvnURL( repos );
	recordCurrentURL( nurl );
	QString dpath = dest.path();

	//find the requested revision
	svn_opt_revision_t rev = createRevision( revnumber, revkind, subpool );

	initNotifier(true, false, false, subpool);
	svn_error_t *err = svn_client_checkout (NULL/* rev actually checkedout */, svn_path_canonicalize( target.toUtf8(), subpool ), svn_path_canonicalize ( dpath.toUtf8(), subpool ), &rev, true, ctx, subpool);
	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::commit(const KUrl::List& wc) {
	kDebug(7128) << "kio_svnProtocol::commit() : " << wc;

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_client_commit_info_t *commit_info = NULL;
	bool nonrecursive = false;

	apr_array_header_t *targets = apr_array_make(subpool, 1+wc.count(), sizeof(const char *));

	for ( QList<KUrl>::const_iterator it = wc.begin(); it != wc.end() ; ++it ) {
		KUrl nurl = *it;
		nurl.setProtocol( "file" );
		recordCurrentURL( nurl );
		(*(( const char ** )apr_array_push(( apr_array_header_t* )targets)) ) = svn_path_canonicalize( nurl.path().toUtf8(), subpool );
	}

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_commit(&commit_info,targets,nonrecursive,ctx,subpool);
	if ( err ) {
		error( KIO::ERR_SLAVE_DEFINED, err->message );
		svn_pool_destroy (subpool);
		return;
	}

	if ( commit_info ) {
		for ( QList<KUrl>::const_iterator it = wc.begin(); it != wc.end() ; ++it ) {
			KUrl nurl = *it;
			nurl.setProtocol( "file" );

			QString userstring = i18n ( "Nothing to commit." );
			if ( SVN_IS_VALID_REVNUM( commit_info->revision ) )
				userstring = i18n( "Committed revision %1." , commit_info->revision);
			setMetaData(QString::number( m_counter ).rightJustified( 10,'0' )+ "path", nurl.path() );
			setMetaData(QString::number( m_counter ).rightJustified( 10,'0' )+ "action", "0" );
			setMetaData(QString::number( m_counter ).rightJustified( 10,'0' )+ "kind", "0" );
			setMetaData(QString::number( m_counter ).rightJustified( 10,'0' )+ "mime_t", "" );
			setMetaData(QString::number( m_counter ).rightJustified( 10,'0' )+ "content", "0" );
			setMetaData(QString::number( m_counter ).rightJustified( 10,'0' )+ "prop", "0" );
			setMetaData(QString::number( m_counter ).rightJustified( 10,'0' )+ "rev" , QString::number( commit_info->revision ) );
			setMetaData(QString::number( m_counter ).rightJustified( 10,'0' )+ "string", userstring );
			m_counter++;
		}
	}

	finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::add(const KUrl& wc) {
	kDebug(7128) << "kio_svnProtocol::add() : " << wc.url();

	apr_pool_t *subpool = svn_pool_create (pool);
	bool nonrecursive = false;

	KUrl nurl = wc;
	nurl.setProtocol( "file" );
	QString target = nurl.url();
	recordCurrentURL( nurl );

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_add(svn_path_canonicalize( nurl.path().toUtf8(), subpool ),nonrecursive,ctx,subpool);
	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::wc_delete(const KUrl::List& wc) {
	kDebug(7128) << "kio_svnProtocol::wc_delete() : " << wc;

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_client_commit_info_t *commit_info = NULL;
	bool nonrecursive = false;

	apr_array_header_t *targets = apr_array_make(subpool, 1+wc.count(), sizeof(const char *));

	for ( QList<KUrl>::const_iterator it = wc.begin(); it != wc.end() ; ++it ) {
		KUrl nurl = *it;
		nurl.setProtocol( "file" );
		recordCurrentURL( nurl );
		(*(( const char ** )apr_array_push(( apr_array_header_t* )targets)) ) = svn_path_canonicalize( nurl.path().toUtf8(), subpool );
	}

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_delete(&commit_info,targets,nonrecursive,ctx,subpool);

	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::wc_revert(const KUrl::List& wc) {
	kDebug(7128) << "kio_svnProtocol::revert() : " << wc;

	apr_pool_t *subpool = svn_pool_create (pool);
	bool nonrecursive = false;

	apr_array_header_t *targets = apr_array_make(subpool, 1 + wc.count(), sizeof(const char *));

	for ( QList<KUrl>::const_iterator it = wc.begin(); it != wc.end() ; ++it ) {
		KUrl nurl = *it;
		nurl.setProtocol( "file" );
		recordCurrentURL( nurl );
		(*(( const char ** )apr_array_push(( apr_array_header_t* )targets)) ) = svn_path_canonicalize( nurl.path().toUtf8(), subpool );
	}

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_revert(targets,nonrecursive,ctx,subpool);
	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

void kio_svnProtocol::wc_status(const KUrl& wc, bool checkRepos, bool fullRecurse, bool getAll, int revnumber, const QString& revkind) {
	kDebug(7128) << "kio_svnProtocol::status() : " << wc.url();

	apr_pool_t *subpool = svn_pool_create (pool);
	svn_revnum_t result_rev;
	bool no_ignore = false;

	KUrl nurl = wc;
	nurl.setProtocol( "file" );
	recordCurrentURL( nurl );

	svn_opt_revision_t rev = createRevision( revnumber, revkind, subpool );

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_status(&result_rev, svn_path_canonicalize( nurl.path().toUtf8(), subpool ), &rev, kio_svnProtocol::status, this, fullRecurse, getAll, checkRepos, no_ignore, ctx, subpool);
	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

//change the proto and remove trailing /
//remove double / also
QString kio_svnProtocol::makeSvnURL ( const KUrl& url ) const {
	QString kproto = url.protocol();
	KUrl tpURL = url;
	tpURL.cleanPath( KUrl::SimplifyDirSeparators );
	QString svnUrl;
	if ( kproto == "svn+http" ) {
		kDebug(7128) << "http:/ " << url.url();
		tpURL.setProtocol("http");
		svnUrl = tpURL.url( KUrl::RemoveTrailingSlash );
		return svnUrl;
	}
	else if ( kproto == "svn+https" ) {
		kDebug(7128) << "https:/ " << url.url();
		tpURL.setProtocol("https");
		svnUrl = tpURL.url( KUrl::RemoveTrailingSlash );
		return svnUrl;
	}
	else if ( kproto == "svn+ssh" ) {
		kDebug(7128) << "svn+ssh:/ " << url.url();
		tpURL.setProtocol("svn+ssh");
		svnUrl = tpURL.url( KUrl::RemoveTrailingSlash );
		return svnUrl;
	}
	else if ( kproto == "svn" ) {
		kDebug(7128) << "svn:/ " << url.url();
		tpURL.setProtocol("svn");
		svnUrl = tpURL.url( KUrl::RemoveTrailingSlash );
		return svnUrl;
	}
	else if ( kproto == "svn+file" ) {
		kDebug(7128) << "file:/ " << url.url();
		tpURL.setProtocol("file");
		svnUrl = tpURL.url( KUrl::RemoveTrailingSlash );
		//hack : add one more / after file:/
		int idx = svnUrl.indexOf('/');
		svnUrl.insert( idx, "//" );
		return svnUrl;
	}
	return tpURL.url( KUrl::RemoveTrailingSlash );
}

QString kio_svnProtocol::chooseProtocol ( const QString& kproto ) const {
	if ( kproto == "svn+http" ) return QString( "http" );
	else if ( kproto == "svn+https" ) return QString( "https" );
	else if ( kproto == "svn+ssh" ) return QString( "svn+ssh" );
	else if ( kproto == "svn" ) return QString( "svn" );
	else if ( kproto == "svn+file" ) return QString( "file" );
	return kproto;
}

svn_error_t *kio_svnProtocol::trustSSLPrompt(svn_auth_cred_ssl_server_trust_t **cred_p, void *, const char */*realm*/, apr_uint32_t /*failures*/, const svn_auth_ssl_server_cert_info_t */*cert_info*/, svn_boolean_t /*may_save*/, apr_pool_t *pool) {
	//when ksvnd is ready make it prompt for the SSL certificate ... XXX
	*cred_p = (svn_auth_cred_ssl_server_trust_t*)apr_pcalloc (pool, sizeof (**cred_p));
	(*cred_p)->may_save = false;
	return SVN_NO_ERROR;
}

svn_error_t *kio_svnProtocol::clientCertSSLPrompt(svn_auth_cred_ssl_client_cert_t **/*cred_p*/, void *, const char */*realm*/, svn_boolean_t /*may_save*/, apr_pool_t */*pool*/) {
	//when ksvnd is ready make it prompt for the SSL certificate ... XXX
/*	*cred_p = apr_palloc (pool, sizeof(**cred_p));
	(*cred_p)->cert_file = cert_file;*/
	return SVN_NO_ERROR;
}

svn_error_t *kio_svnProtocol::clientCertPasswdPrompt(svn_auth_cred_ssl_client_cert_pw_t **/*cred_p*/, void *, const char */*realm*/, svn_boolean_t /*may_save*/, apr_pool_t */*pool*/) {
	//when ksvnd is ready make it prompt for the SSL certificate password ... XXX
	return SVN_NO_ERROR;
}

svn_error_t *kio_svnProtocol::commitLogPrompt( const char **log_msg, const char **/*file*/, apr_array_header_t *commit_items, void *baton, apr_pool_t *pool ) {
	QString result;
	QStringList slist;
	kio_svnProtocol *p = ( kio_svnProtocol* )baton;
	svn_stringbuf_t *message = NULL;

	for (int i = 0; i < commit_items->nelts; i++) {
		QString list;
		svn_client_commit_item_t *item = ((svn_client_commit_item_t **) commit_items->elts)[i];
		const char *path = item->path;
		char text_mod = '_', prop_mod = ' ';

		if (! path)
			path = item->url;
		else if (! *path)
			path = ".";

		if (! path)
			path = ".";

		if ((item->state_flags & SVN_CLIENT_COMMIT_ITEM_DELETE) && (item->state_flags & SVN_CLIENT_COMMIT_ITEM_ADD))
			text_mod = 'R';
		else if (item->state_flags & SVN_CLIENT_COMMIT_ITEM_ADD)
			text_mod = 'A';
		else if (item->state_flags & SVN_CLIENT_COMMIT_ITEM_DELETE)
			text_mod = 'D';
		else if (item->state_flags & SVN_CLIENT_COMMIT_ITEM_TEXT_MODS)
			text_mod = 'M';
		if (item->state_flags & SVN_CLIENT_COMMIT_ITEM_PROP_MODS)
			prop_mod = 'M';

		list += text_mod;
		list += " ";
		list += prop_mod;
		list += "  ";
		list += path;
		kDebug(7128) << " Committing items : " << list;
		slist << list;
	}
	OrgKdeKsvndInterface ksvndInterface( "org.kde.kded", "/modules/ksvnd", QDBusConnection::sessionBus() );
	if(!ksvndInterface.isValid())
	{
	   kWarning() << "Communication with KDED:KSvnd failed";
	   return SVN_NO_ERROR;
	}

	QString lst = slist.join("\n");
	QDBusReply<QString> reply = ksvndInterface.commitDialog(lst);
	if ( !reply.isValid() ) 
	{
		kWarning() << "Unexpected reply type";
		return SVN_NO_ERROR;
	}
	result = reply;
	if ( result.isNull() ) { //cancelled
		*log_msg = NULL;
		return SVN_NO_ERROR;
	}
	message = svn_stringbuf_create( result.toUtf8(), pool );
	*log_msg = message->data;
	return SVN_NO_ERROR;
}

void kio_svnProtocol::notify(void *baton, const char *path, svn_wc_notify_action_t action, svn_node_kind_t kind, const char *mime_type, svn_wc_notify_state_t content_state, svn_wc_notify_state_t prop_state, svn_revnum_t revision) {
	kDebug(7128) << "NOTIFY : " << path << " updated at revision " << revision << " action : " << action << ", kind : " << kind << " , content_state : " << content_state << ", prop_state : " << prop_state;

	QString userstring;
	struct notify_baton *nb = ( struct notify_baton* ) baton;

	//// Convert notification to a user readable string
	switch ( action ) {
		case svn_wc_notify_add : //add
			if (mime_type && (svn_mime_type_is_binary (mime_type)))
				userstring = i18n( "A (bin) %1", path );
			else
				userstring = i18n( "A %1", path );
			break;
		case svn_wc_notify_copy: //copy
			break;
		case svn_wc_notify_delete: //delete
			nb->received_some_change = true;
			userstring = i18n( "D %1", path );
			break;
		case svn_wc_notify_restore : //restore
			userstring=i18n( "Restored %1.", path );
			break;
		case svn_wc_notify_revert : //revert
			userstring=i18n( "Reverted %1.", path );
			break;
		case svn_wc_notify_failed_revert: //failed revert
			userstring=i18n( "Failed to revert %1.\nTry updating instead.", path );
			break;
		case svn_wc_notify_resolved: //resolved
			userstring=i18n( "Resolved conflicted state of %1.", path );
			break;
		case svn_wc_notify_skip: //skip
			if ( content_state == svn_wc_notify_state_missing )
				userstring=i18n("Skipped missing target %1.", path );
			else
				userstring=i18n("Skipped  %1.", path );
			break;
		case svn_wc_notify_update_delete: //update_delete
			nb->received_some_change = true;
			userstring=i18n( "D %1", path );
			break;
		case svn_wc_notify_update_add: //update_add
			nb->received_some_change = true;
			userstring=i18n( "A %1", path );
			break;
		case svn_wc_notify_update_update: //update_update
			{
				/* If this is an inoperative dir change, do no notification.
				   An inoperative dir change is when a directory gets closed
				   without any props having been changed. */
				if (! ((kind == svn_node_dir)
							&& ((prop_state == svn_wc_notify_state_inapplicable)
								|| (prop_state == svn_wc_notify_state_unknown)
								|| (prop_state == svn_wc_notify_state_unchanged)))) {
					nb->received_some_change = true;

					if (kind == svn_node_file) {
						if (content_state == svn_wc_notify_state_conflicted)
							userstring = "C";
						else if (content_state == svn_wc_notify_state_merged)
							userstring = "G";
						else if (content_state == svn_wc_notify_state_changed)
							userstring = "U";
					}

					if (prop_state == svn_wc_notify_state_conflicted)
						userstring += "C";
					else if (prop_state == svn_wc_notify_state_merged)
						userstring += "G";
					else if (prop_state == svn_wc_notify_state_changed)
						userstring += "U";
					else
						userstring += " ";

					if (! ((content_state == svn_wc_notify_state_unchanged
									|| content_state == svn_wc_notify_state_unknown)
								&& (prop_state == svn_wc_notify_state_unchanged
									|| prop_state == svn_wc_notify_state_unknown)))
						userstring += QString( " " ) + path;
				}
				break;
			}
		case svn_wc_notify_update_completed: //update_completed
			{
				if (! nb->suppress_final_line) {
					if (SVN_IS_VALID_REVNUM (revision)) {
						if (nb->is_export) {
							if ( nb->in_external )
								userstring = i18n("Exported external at revision %1.", revision );
							else
								userstring = i18n("Exported revision %1.", revision );
						} else if (nb->is_checkout) {
							if ( nb->in_external )
								userstring = i18n("Checked out external at revision %1.", revision );
							else
								userstring = i18n("Checked out revision %1.", revision);
						} else {
							if (nb->received_some_change) {
								if ( nb->in_external )
									userstring=i18n("Updated external to revision %1.", revision );
								else
									userstring = i18n("Updated to revision %1.", revision);
							} else {
								if ( nb->in_external )
									userstring = i18n("External at revision %1.", revision );
								else
									userstring = i18n("At revision %1.", revision);
							}
						}
					} else  /* no revision */ {
						if (nb->is_export) {
							if ( nb->in_external )
								userstring = i18n("External export complete.");
							else
								userstring = i18n("Export complete.");
						} else if (nb->is_checkout) {
							if ( nb->in_external )
								userstring = i18n("External checkout complete.");
							else
								userstring = i18n("Checkout complete.");
						} else {
							if ( nb->in_external )
								userstring = i18n("External update complete.");
							else
								userstring = i18n("Update complete.");
						}
					}
				}
			}
			if (nb->in_external)
				nb->in_external = false;
			break;
		case svn_wc_notify_update_external: //update_external
			nb->in_external = true;
			userstring = i18n("Fetching external item into %1.", path );
			break;
		case svn_wc_notify_status_completed: //status_completed
			if (SVN_IS_VALID_REVNUM (revision))
				userstring = i18n( "Status against revision: %1.", revision );
			break;
		case svn_wc_notify_status_external: //status_external
             userstring = i18n("Performing status on external item at %1.", path );
			break;
		case svn_wc_notify_commit_modified: //commit_modified
			userstring = i18n( "Sending %1", path );
			break;
		case svn_wc_notify_commit_added: //commit_added
			if (mime_type && svn_mime_type_is_binary (mime_type)) {
				userstring = i18n( "Adding (bin) %1.", path );
			} else {
				userstring = i18n( "Adding %1.", path );
			}
			break;
		case svn_wc_notify_commit_deleted: //commit_deleted
			userstring = i18n( "Deleting %1.", path );
			break;
		case svn_wc_notify_commit_replaced: //commit_replaced
			userstring = i18n( "Replacing %1.", path );
			break;
		case svn_wc_notify_commit_postfix_txdelta: //commit_postfix_txdelta
			if (! nb->sent_first_txdelta) {
				nb->sent_first_txdelta = true;
				userstring=i18n("Transmitting file data ");
			} else {
				userstring=".";
			}
			break;

			break;
		case svn_wc_notify_blame_revision: //blame_revision
			break;
		default:
			break;
	}
	//// End convert

	kio_svnProtocol *p = ( kio_svnProtocol* )nb->master;

	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "path" , QString::fromUtf8( path ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "action", QString::number( action ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "kind", QString::number( kind ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "mime_t", QString::fromUtf8( mime_type ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "content", QString::number( content_state ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "prop", QString::number( prop_state ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "rev", QString::number( revision ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "string", userstring );
	p->incCounter();
}

void kio_svnProtocol::status(void *baton, const char *path, svn_wc_status_t *status) {
	kDebug(7128) << "STATUS : " << path << ", wc text status : " << status->text_status
									 << ", wc prop status : " << status->prop_status
									 << ", repos text status : " << status->repos_text_status
									 << ", repos prop status : " << status->repos_prop_status
									 << endl;

	QByteArray params;
	kio_svnProtocol *p = ( kio_svnProtocol* )baton;

	QDataStream stream(&params, QIODevice::WriteOnly);
	long int rev = status->entry ? status->entry->revision : 0;
	stream << QString::fromUtf8( path ) << QString::number( status->text_status ) << QString::number( status->prop_status ) << QString::number( status->repos_text_status ) << QString::number( status->repos_prop_status ) << QString::number( rev );

	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "path", QString::fromUtf8( path ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "text", QString::number( status->text_status ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "prop", QString::number( status->prop_status ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "reptxt", QString::number( status->repos_text_status ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "repprop", QString::number( status->repos_prop_status ));
	p->setMetaData(QString::number( p->counter() ).rightJustified( 10,'0' )+ "rev", QString::number( rev ));
	p->incCounter();
}


void kio_svnProtocol::wc_resolve( const KUrl& wc, bool recurse ) {
	kDebug(7128) << "kio_svnProtocol::wc_resolve() : " << wc.url();

	apr_pool_t *subpool = svn_pool_create (pool);

	KUrl nurl = wc;
	nurl.setProtocol( "file" );
	recordCurrentURL( nurl );

	initNotifier(false, false, false, subpool);
	svn_error_t *err = svn_client_resolved(svn_path_canonicalize( nurl.path().toUtf8(), subpool ), recurse,ctx,subpool);
	if ( err )
		error( KIO::ERR_SLAVE_DEFINED, err->message );
	else
		finished();
	svn_pool_destroy (subpool);
}

extern "C"
{
	KDE_EXPORT int kdemain(int argc, char **argv)    {
		KComponentData componentData( "kio_svn" );

		kDebug(7128) << "*** Starting kio_svn ";

		if (argc != 4) {
			kDebug(7128) << "Usage: kio_svn  protocol domain-socket1 domain-socket2";
			exit(-1);
		}

		kio_svnProtocol slave(argv[2], argv[3]);
		slave.dispatchLoop();

		kDebug(7128) << "*** kio_svn Done";
		return 0;
	}
}

