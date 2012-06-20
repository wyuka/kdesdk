/* 
 *  Copyright (c) 1999-2002 Bernd Gehrmann <bernd@mail.berlios.de>
 *  Copyright (c) 2002      Christian Loose <christian.loose@hamburg.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <kdialog.h>
#include <QDBusReply>

class QString;
class QWidget;
class QDBusObjectPath;
class ProgressDialog : public KDialog
{
    Q_OBJECT

public:
    ProgressDialog(QWidget* parent, const QString& heading, const QString &cvsServiceNameService,const QDBusReply<QDBusObjectPath>& job,
                   const QString& errorIndicator, const QString& caption = "");
    ~ProgressDialog();

    bool execute();
    bool getLine(QString& line);
    QStringList getOutput() const;

public Q_SLOTS:
    void slotReceivedOutputNonGui(QString buffer);
    void slotReceivedOutput(QString buffer);
    void slotJobExited(bool normalExit, int status);

protected slots:
    virtual void slotCancel();

private slots:
    void slotTimeoutOccurred();

private:
    void setupGui(const QString& heading);
    void stopNonGuiPart();
    void startGuiPart();
    void processOutput();

    struct Private;
    Private* d;
};

#endif // PROGRESSDIALOG_H
