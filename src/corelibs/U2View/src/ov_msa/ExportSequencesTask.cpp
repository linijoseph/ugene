/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2017 UniPro <ugene@unipro.ru>
 * http://ugene.net
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

#include "ExportSequencesTask.h"

#include <QFile>

#include <U2Core/AppContext.h>
#include <U2Core/DNASequenceObject.h>
#include <U2Core/DocumentSelection.h>
#include <U2Core/GUrlUtils.h>
#include <U2Core/IOAdapterUtils.h>
#include <U2Core/MultiTask.h>
#include <U2Core/MSAUtils.h>
#include <U2Core/ProjectModel.h>
#include <U2Core/SaveDocumentTask.h>
#include <U2Core/U2ObjectDbi.h>

#include <U2Gui/ObjectViewModel.h>

#include <U2Formats/DocumentFormatUtils.h>

namespace U2 {

PrepareSequenceObjectsTask::PrepareSequenceObjectsTask(const MultipleSequenceAlignment& msa, const QStringList& seqNames, bool trimGaps) : Task(tr("Prepare sequences"), TaskFlag_None),
    msa(msa),
    seqNames(seqNames),
    trimGaps(trimGaps)
{}

void PrepareSequenceObjectsTask::run() {
    foreach(const DNASequence &s, MSAUtils::ma2seq(msa,trimGaps)) {
        DNASequence seq;
        if (!seqNames.contains(s.getName())) {
            continue;
        }
        sequences.append(s);
    }
}

QList<DNASequence> PrepareSequenceObjectsTask::getSequences() {
    return sequences;
}

ExportSequencesTask::ExportSequencesTask(const MultipleSequenceAlignment& msa, const QStringList& seqNames, bool trimGaps, bool addToProjectFlag,
    const QString& dirUrl, const DocumentFormatId& format, const QString& extension, const QString& customFileName) : Task(tr("Export selected sequences form alignment"), TaskFlags_NR_FOSE_COSC),
    msa(msa),
    seqNames(seqNames),
    trimGaps(trimGaps),
    addToProjectFlag(addToProjectFlag),
    dirUrl(dirUrl),
    format(format),
    extension(extension),
    customFileName(customFileName)
{}

void ExportSequencesTask::prepare() {
    prepareObjectsTask = new PrepareSequenceObjectsTask(msa, seqNames, trimGaps);
    addSubTask(prepareObjectsTask);
}

QList<Task*> ExportSequencesTask::onSubTaskFinished(Task* subTask) {
    QList<Task*> res;
    CHECK_OP(stateInfo, res);

    if (subTask == prepareObjectsTask) {
        QList<Task*> tasks;
        QSet<QString> existingFilenames;
        foreach(const DNASequence &s, prepareObjectsTask->getSequences()) {
            QString filename;
            if (customFileName.isEmpty()) {
                filename = GUrlUtils::fixFileName(s.getName());
            } else {
                filename = GUrlUtils::fixFileName(customFileName);
            }
            QString filePath = GUrlUtils::prepareFileLocation(dirUrl + "/" + filename + "." + extension, stateInfo);
            CHECK_OP(stateInfo, res);

            QFile fileToSave(filePath);
            filePath = GUrlUtils::rollFileName(filePath, "_", existingFilenames);
            existingFilenames.insert(filePath);
            GUrl url(filePath);
            IOAdapterFactory* iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(IOAdapterUtils::url2io(url));
            DocumentFormat *df = AppContext::getDocumentFormatRegistry()->getFormatById(format);
            QList<GObject*> objs;
            Document *doc = df->createNewLoadedDocument(iof, filePath, stateInfo);
            CHECK_OP_EXT(stateInfo, delete doc, res);
            DNASequence seq = s;
            U2SequenceObject* seqObj = DocumentFormatUtils::addSequenceObjectDeprecated(doc->getDbiRef(), U2ObjectDbi::ROOT_FOLDER, seq.getName(), objs, seq, stateInfo);
            CHECK_OP_EXT(stateInfo, delete doc, res);
            doc->addObject(seqObj);
            SaveDocumentTask *t = new SaveDocumentTask(doc, doc->getIOAdapterFactory(), doc->getURL());

            if (addToProjectFlag) {
                Project *p = AppContext::getProject();
                Document *loadedDoc = p->findDocumentByURL(url);
                if (loadedDoc) {
                    coreLog.details("The document already in the project");
                    return res;
                }
                p->addDocument(doc);

                // Open view for created document
                DocumentSelection ds;
                ds.setSelection(QList<Document*>() << doc);
                MultiGSelection ms;
                ms.addSelection(&ds);
                foreach(GObjectViewFactory *f, AppContext::getObjectViewFactoryRegistry()->getAllFactories()) {
                    if (f->canCreateView(ms)) {
                        Task *tt = f->createViewTask(ms);
                        AppContext::getTaskScheduler()->registerTopLevelTask(tt);
                        break;
                    }
                }
            } else {
                t->addFlag(SaveDoc_DestroyAfter);
            }
            tasks.append(t);
        }
        res.append(new MultiTask(tr("Save sequences from alignment"), tasks));
    }
    return res;
}

}
