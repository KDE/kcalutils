/*
  This file is part of the kcal library.

  Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Author: Kevin Krammer, krake@kdab.com

  Copyright (C) 2015 Sérgio Martins <iamsergio@gmail.com>

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

#include "recurrenceactions.h"

#include "ui_recurrenceactionsscopewidget.h"

#include <KLocale>
#include <KLocalizedString>
#include <KMessageBox>
#include <KConfigGroup>
#include <KGuiItem>

#include <QPointer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <boost/shared_ptr.hpp>

using namespace KCalUtils;
using namespace KCalUtils::RecurrenceActions;
using namespace KCalCore;

class ScopeWidget : public QWidget
{
public:
    ScopeWidget(int availableChoices, const KDateTime &dateTime, QWidget *parent)
        : QWidget(parent), mAvailableChoices(availableChoices)
    {
        mUi.setupUi(this);

        if ((mAvailableChoices & PastOccurrences) == 0) {
            mUi.checkBoxPast->hide();
        } else {
            mUi.checkBoxPast->setText(i18nc("@option:check calendar items before a certain date",
                                            "Items before %1",
                                            KLocale::global()->formatDateTime(dateTime)));
        }
        if ((mAvailableChoices & SelectedOccurrence) == 0) {
            mUi.checkBoxSelected->hide();
        } else {
            mUi.checkBoxSelected->setText(i18nc("@option:check currently selected calendar item",
                                                "Selected item"));
        }
        if ((mAvailableChoices & FutureOccurrences) == 0) {
            mUi.checkBoxFuture->hide();
        } else {
            mUi.checkBoxFuture->setText(i18nc("@option:check calendar items after a certain date",
                                              "Items after %1",
                                              KLocale::global()->formatDateTime(dateTime)));
        }
    }

    void setMessage(const QString &message);
    void setIcon(const QIcon &icon);

    void setCheckedChoices(int choices);
    int checkedChoices() const;

private:
    const int mAvailableChoices;
    Ui_RecurrenceActionsScopeWidget mUi;
};

void ScopeWidget::setMessage(const QString &message)
{
    mUi.messageLabel->setText(message);
}

void ScopeWidget::setIcon(const QIcon &icon)
{
    QStyleOption option;
    option.initFrom(this);
    mUi.iconLabel->setPixmap(
        icon.pixmap(style()->pixelMetric(QStyle::PM_MessageBoxIconSize, &option, this)));
}

void ScopeWidget::setCheckedChoices(int choices)
{
    // mask with available ones
    choices &= mAvailableChoices;

    mUi.checkBoxPast->setChecked((choices & PastOccurrences) != 0);
    mUi.checkBoxSelected->setChecked((choices & SelectedOccurrence) != 0);
    mUi.checkBoxFuture->setChecked((choices & FutureOccurrences) != 0);
}

int ScopeWidget::checkedChoices() const
{
    int result = NoOccurrence;

    if (mUi.checkBoxPast->isChecked()) {
        result |= PastOccurrences;
    }
    if (mUi.checkBoxSelected->isChecked()) {
        result |= SelectedOccurrence;
    }
    if (mUi.checkBoxFuture->isChecked()) {
        result |= FutureOccurrences;
    }

    return result;
}

int RecurrenceActions::availableOccurrences(const Incidence::Ptr &incidence,
        const KDateTime &selectedOccurrence)
{
    int result = NoOccurrence;

    if (incidence->recurrence()->recursOn(selectedOccurrence.date(),
                                          selectedOccurrence.timeSpec())) {
        result |= SelectedOccurrence;
    }

    if (incidence->recurrence()->getPreviousDateTime(selectedOccurrence).isValid()) {
        result |= PastOccurrences;
    }

    if (incidence->recurrence()->getNextDateTime(selectedOccurrence).isValid()) {
        result |= FutureOccurrences;
    }

    return result;
}

int RecurrenceActions::questionMultipleChoice(const KDateTime &selectedOccurrence,
        const QString &message, const QString &caption,
        const KGuiItem &action, int availableChoices,
        int preselectedChoices, QWidget *parent)
{
    QPointer<QDialog> dialog = new QDialog(parent);
    dialog->setWindowTitle(caption);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(dialog);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    dialog->setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    KGuiItem::assign(okButton, action);

    ScopeWidget *widget = new ScopeWidget(availableChoices, selectedOccurrence, dialog);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);

    widget->setMessage(message);
    widget->setIcon(widget->style()->standardIcon(QStyle::SP_MessageBoxQuestion));
    widget->setCheckedChoices(preselectedChoices);

    const int result = dialog->exec();
    if (dialog) {
        dialog->deleteLater();
    }

    if (result == QDialog::Rejected) {
        return NoOccurrence;
    }

    return widget->checkedChoices();
}

int RecurrenceActions::questionSelectedAllCancel(const QString &message, const QString &caption,
        const KGuiItem &actionSelected,
        const KGuiItem &actionAll, QWidget *parent)
{
    QDialog *dialog = new QDialog(parent);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    dialog->setLayout(mainLayout);
    dialog->setWindowTitle(caption);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::No | QDialogButtonBox::Yes);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    dialog->setObjectName(QLatin1String("RecurrenceActions::questionSelectedAllCancel"));
    buttonBox->button(QDialogButtonBox::Yes)->setDefault(true);
    KGuiItem::assign(buttonBox->button(QDialogButtonBox::Yes), actionSelected);
    KGuiItem::assign(okButton, actionAll);

    // Qt5: Port
//   bool checkboxResult = false;
//   int result = KMessageBox::createKMessageBox(
//     dialog,
//     QMessageBox::Question,
//     message,
//     QStringList(),
//     QString(),
//     &checkboxResult,
//     KMessageBox::Notify );
//
//   switch (result) {
//     case QDialog::Yes:
//       return SelectedOccurrence;
//     case QDialog::Accepted:
//       // See kdialog.h, 'Ok' doesn't return QDialog:Ok
//       return AllOccurrences;
//     default:
//       return NoOccurrence;
//   }

    return NoOccurrence;
}

int RecurrenceActions::questionSelectedFutureAllCancel(const QString &message,
        const QString &caption,
        const KGuiItem &actionSelected,
        const KGuiItem &actionFuture,
        const KGuiItem &actionAll,
        QWidget *parent)
{
    QDialog *dialog = new QDialog(parent);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    dialog->setLayout(mainLayout);
    dialog->setWindowTitle(caption);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::No | QDialogButtonBox::Yes);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    dialog->setObjectName(QLatin1String("RecurrenceActions::questionSelectedFutureAllCancel"));
    buttonBox->button(QDialogButtonBox::Yes)->setDefault(true);
    KGuiItem::assign(buttonBox->button(QDialogButtonBox::Yes), actionSelected);
    KGuiItem::assign(buttonBox->button(QDialogButtonBox::No), actionFuture);
    KGuiItem::assign(okButton, actionAll);

    // Qt5: Port
//   bool checkboxResult = false;
//   int result = KMessageBox::createKMessageBox(
//     dialog,
//     QMessageBox::Question,
//     message,
//     QStringList(),
//     QString(),
//     &checkboxResult,
//     KMessageBox::Notify );
//
//   switch (result) {
//     case QDialog::Yes:
//       return SelectedOccurrence;
//     case QDialog::No:
//       return FutureOccurrences;
//     case QDialog::Accepted:
//       return AllOccurrences;
//     default:
//       return NoOccurrence;
//   }

    return NoOccurrence;
}

