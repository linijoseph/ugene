/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2012 UniPro <ugene@unipro.ru>
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

#ifndef _U2_GUI_DOCUMENT_UTILS_H_
#define _U2_GUI_DOCUMENT_UTILS_H_

#include <U2Core/U2OpStatus.h>
#include <U2Test/GUITestBase.h>

namespace U2 {

class GObjectView;
class Document;

class DocumentUtils {
public:
    // checks if the document with a given name is loaded in a view with a given factory Id
    static void checkDocumentExists(U2OpStatus &os, const QString &documentName, const GObjectViewFactoryId &id = QString());
    GENERATE_GUI_ACTION_2(CheckDocumentExistsGUIAction, checkDocumentExists);

    class RemoveDocumentGUIAction : public GUIMultiTest {
    public:
        RemoveDocumentGUIAction(const QString &documentName);
    };

    static Document* getDocument(U2OpStatus &os, const QString& documentName);

protected:
    static GObjectView* getDocumentGObjectView(U2OpStatus &os, Document* d);

private:
    static QList<GObjectView*> getAllGObjectViews();
};

} // namespace

#endif
