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

#include <U2Core/AppContext.h>
#include <U2Core/MSAUtils.h>

namespace U2 {

PrepareSequenceObjectsTask::PrepareSequenceObjectsTask(MultipleSequenceAlignmentObject *_maObj, const QStringList& _seqNames, bool _trimGaps) : Task(tr("Prepare sequences"), TaskFlag_None),
    maObj(_maObj),
    seqNames(_seqNames),
    trimGaps(_trimGaps)
{}

void PrepareSequenceObjectsTask::run() {
    foreach(const DNASequence &s, MSAUtils::ma2seq(maObj->getMsa(),trimGaps)) {
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

ExportSequencesTask::ExportSequencesTask(MultipleSequenceAlignmentObject *_maObj, const QStringList& _seqNames, bool _trimGaps) : Task(tr("Export selected sequences form alignment"), TaskFlags_NR_FOSE_COSC),
maObj(_maObj),
seqNames(_seqNames),
trimGaps(_trimGaps) {}

void ExportSequencesTask::init() {
    prepareObjectsTask = new PrepareSequenceObjectsTask(maObj, seqNames, trimGaps);
    AppContext::getTaskScheduler()->registerTopLevelTask(prepareObjectsTask);
}

QList<Task*> ExportSequencesTask::onSubTaskFinished(Task* subTask) {
    QList<Task*> res;
    CHECK_OP(stateInfo, res);

    if (subTask == prepareObjectsTask) {
        QList<Task*> tasks;
        QSet<QString> existingFilenames;
        foreach(const DNASequence &s, prepareObjectsTask->getSequences()) {
            QString filePath;
            if (customFileName.isEmpty()) {
                filePath = GUrlUtils::prepareFileLocation(url + QDir::separator() + s.getName() + "." + extension, stateInfo);
            } else {
                filePath = GUrlUtils::prepareFileLocation(url + QDir::separator() + customFileName + "." + extension, stateInfo);
            }
            CHECK_OP(stateInfo, );
            GUrlUtils::fixFileName(filePath);
            QFile fileToSave(filePath);
            filePath = GUrlUtils::rollFileName(filePath, "_", existingFilenames);
            existingFilenames.insert(filePath);
            GUrl url(filePath);
            IOAdapterFactory* iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(IOAdapterUtils::url2io(d->url));
            DocumentFormat *df = AppContext::getDocumentFormatRegistry()->getFormatById(d->format);
            QList<GObject*> objs;
            Document *doc = df->createNewLoadedDocument(iof, filePath, stateInfo);
            CHECK_OP_EXT(stateInfo, delete doc, );
            U2SequenceObject* seqObj = DocumentFormatUtils::addSequenceObjectDeprecated(doc->getDbiRef(), U2ObjectDbi::ROOT_FOLDER, seq.getName(), objs, seq, stateInfo);
            CHECK_OP_EXT(stateInfo, delete doc, );
            doc->addObject(seqObj);
            SaveDocumentTask *t = new SaveDocumentTask(doc, doc->getIOAdapterFactory(), doc->getURL());

            if (d->addToProjectFlag) {
                Project *p = AppContext::getProject();
                Document *loadedDoc = p->findDocumentByURL(url);
                if (loadedDoc) {
                    coreLog.details("The document already in the project");
                    QMessageBox::warning(ui, tr("warning"), tr("The document already in the project"));
                    return;
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
    }
    return res;
}

}
