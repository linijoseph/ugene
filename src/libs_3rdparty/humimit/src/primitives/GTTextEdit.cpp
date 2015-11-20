/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2015 UniPro <ugene@unipro.ru>
 * http://ugene.unipro.ru
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <drivers/GTKeyboardDriver.h>
#include "primitives/GTTextEdit.h"
#include <primitives/GTWidget.h>
#include "utils/GTKeyboardUtils.h"

namespace HI {

#define GT_CLASS_NAME "GTTextEdit"

#define GT_METHOD_NAME ""
void GTTextEdit::setText(U2::U2OpStatus& os, QTextEdit* textEdit, const QString &text) {

    GT_CHECK(textEdit != NULL, "plainTextEdit is NULL");

    if(textEdit->toPlainText() == text){
        return;
    }

    clear(os, textEdit);
    GTWidget::setFocus(os, textEdit);

    GTKeyboardDriver::keySequence(os, text);
    GTGlobals::sleep(500);
}
#undef GT_METHOD_NAME

#define GT_METHOD_NAME "getText"
QString GTTextEdit::getText(U2::U2OpStatus &os, QTextEdit *textEdit) {
    Q_UNUSED(os);
    GT_CHECK_RESULT(NULL != textEdit, "Text edit is NULL", "");
    return textEdit->toPlainText();
}
#undef GT_METHOD_NAME

#define GT_METHOD_NAME "containsString"
bool GTTextEdit::containsString(U2::U2OpStatus &os, QTextEdit *textEdit, const QString &string) {
    const QString text = getText(os, textEdit);
    return text.contains(string);
}
#undef GT_METHOD_NAME

#define GT_METHOD_NAME "clear"
void GTTextEdit::clear(U2::U2OpStatus& os, QTextEdit* textEdit) {

    GT_CHECK(textEdit != NULL, "textEdit is NULL");

    GTWidget::setFocus(os, textEdit);

    GTKeyboardUtils::selectAll(os);
    GTGlobals::sleep(100);
    GTKeyboardDriver::keyClick(os, GTKeyboardDriver::key["delete"]);
    GTGlobals::sleep(1000);

    QString s = textEdit->toPlainText();
    GT_CHECK(s.isEmpty() == true, "Can't clear text, textEdit is not empty");
    }
#undef GT_METHOD_NAME

#undef GT_CLASS_NAME

}
