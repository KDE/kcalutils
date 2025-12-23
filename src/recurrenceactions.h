/*
  This file is part of the kcal library.

  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kcalutils_export.h"

#include <KCalendarCore/Incidence>

class QDateTime;
class KGuiItem;
class QWidget;

namespace KCalUtils
{
/*!
  \brief Utility functions for dealing with recurrences

  Incidences with recurrencies need to be treated differently than single independent ones.
  For example the user might be given the choice to not only modify a selected occurrence
  of an incidence but also all that follow that one, etc.

  @author Kevin Krammer, krake@kdab.com
  \since 4.6
*/
namespace RecurrenceActions
{
/*!
  \brief Flags for indicating on which occurrences to work on

  Flags can be OR'ed together to get a combined scope.
*/
enum Scope {
    /*!
     Scope does not apply to any occurrence
    */
    NoOccurrence = 0,

    /*!
     Scope does include the given/selected occurrence
    */
    SelectedOccurrence = 1,

    /*!
     Scope does include occurrences before the given/selected occurrence
    */
    PastOccurrences = 2,

    /*!
     Scope does include occurrences after the given/selected occurrence
    */
    FutureOccurrences = 4,

    /*!
     Scope does include all occurrences (past, present and future)
    */
    AllOccurrences = PastOccurrences | SelectedOccurrence | FutureOccurrences
};

/*!
  \brief Checks what scope an action could be applied on for a given incidence

  Checks whether the incidence is occurring on the given date and whether there
  are occurrences in the past and future.

  \a incidence the incidence of which to check recurrences.
  \a selectedOccurrence the date (including timespec) to use as the base occurrence,
  i.e., from which to check for past and future occurrences.

  Returns the #Scope to which actions on the given @incidence can be applied to
*/
[[nodiscard]] KCALUTILS_EXPORT int availableOccurrences(const KCalendarCore::Incidence::Ptr &incidence, const QDateTime &selectedOccurrence);

/*!
  \brief Presents a multiple choice scope selection dialog to the user

  Shows a message box style question dialog with checkboxes for occurrence scope flags
  so the user can be asked specifically which occurrences to apply actions to.

  \a selectedOccurrence the date to use for telling the user which occurrence
  is the selected one.
  \a message the message which explains the change and selection options.
  \a caption the dialog's caption.
  \a action the GUI item to use for the "OK" button.
  \a availableChoices combined #Scope values to select which options should be present.
  \a preselectedChoices combined #Scope values to optionally preselect some of the options
  specified with \a availableChoices.
  \a parent QWidget parent for the dialog.

  Returns the chosen #Scope options, OR'ed together
*/
[[nodiscard]] KCALUTILS_EXPORT int questionMultipleChoice(const QDateTime &selectedOccurrence,
                                                          const QString &message,
                                                          const QString &caption,
                                                          const KGuiItem &action,
                                                          int availableChoices,
                                                          int preselectedChoices,
                                                          QWidget *parent);

/*!
  \brief Presents a message box with two action choices and cancel to the user

  Shows a message box style question dialog with two action scope buttons and cancel.
  This is for quick decisions like whether to only modify a single occurrence or all occurrences.

  \a message the message which explains the change and available options.
  \a caption the dialog's caption.
  \a actionSelected the GUI item to use for the button representing the
  #SelectedOccurrence scope.
  \a actionAll the GUI item to use for the button representing the #AllOccurrences scope.
  \a parent QWidget parent for the dialog.

  \a #NoOccurrence on cancel, #SelectedOccurrence or #AllOccurrences on the respective action.
*/
[[nodiscard]] KCALUTILS_EXPORT int
questionSelectedAllCancel(const QString &message, const QString &caption, const KGuiItem &actionSelected, const KGuiItem &actionAll, QWidget *parent);

/*!
  \brief Presents a message box with three action choices and cancel to the user

  Shows a message box style question dialog with three action scope buttons and cancel.
  This is for quick decisions like whether to only modify a single occurrence, to include
  future or all occurrences.

  Note The calling application code can of course decide to word the future action text
        in a way that it includes the selected occurrence, e.g. "Also Future Items".
        The returned value will still just be #FutureOccurrences so the calling code
        has to include #SelectedOccurrence itself if it passes the value further on

  \a message the message which explains the change and available options.
  \a caption the dialog's caption.
  \a actionSelected the GUI item to use for the button representing the
  #SelectedOccurrence scope.
  \a actionSelected the GUI item to use for the button representing the
  #FutureOccurrences scope.
  \a actionAll the GUI item to use for the button representing the #AllOccurrences scope.
  \a parent QWidget parent for the dialog.

  \a #NoOccurrence on cancel, #SelectedOccurrence, #FutureOccurrences or #AllOccurrences
  on the respective action.
*/
[[nodiscard]] KCALUTILS_EXPORT int questionSelectedFutureAllCancel(const QString &message,
                                                                   const QString &caption,
                                                                   const KGuiItem &actionSelected,
                                                                   const KGuiItem &actionFuture,
                                                                   const KGuiItem &actionAll,
                                                                   QWidget *parent);
}
}
