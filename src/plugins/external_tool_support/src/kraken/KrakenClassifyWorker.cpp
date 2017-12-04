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

#include <U2Core/FailTask.h>
#include <U2Core/MultiTask.h>
#include <U2Core/U2OpStatusUtils.h>

#include <U2Lang/WorkflowMonitor.h>

#include "KrakenClassifyTask.h"
#include "KrakenClassifyWorker.h"
#include "KrakenClassifyWorkerFactory.h"

namespace U2 {
namespace LocalWorkflow {

const QString KrakenClassifyWorker::INPUT_PORT_ID = "in-port";
const QString KrakenClassifyWorker::PAIRED_INPUT_PORT_ID = "in-paired-port";
const QString KrakenClassifyWorker::OUTPUT_PORT_ID = "out-port";

KrakenClassifyWorker::KrakenClassifyWorker(Actor *actor)
    : BaseWorker(actor),
      input(NULL),
      pairedInput(NULL),
      output(NULL)
{

}

void KrakenClassifyWorker::init() {
    input = ports.value(INPUT_PORT_ID);
    pairedInput = ports.value(PAIRED_INPUT_PORT_ID);
    output = ports.value(OUTPUT_PORT_ID);

    pairedReadsInput = (getValue<QString>(KrakenClassifyWorkerFactory::SEQUENCING_READS_ATTR_ID) == KrakenClassifyTaskSettings::PAIRED_END);

    output->addComplement(input);
    input->addComplement(output);
}

Task *KrakenClassifyWorker::tick() {
    if (isReadyToRun()) {
        KrakenClassifyTask *task = new KrakenClassifyTask(getSettings());
        task->addListeners(createLogListeners(2));
        connect(task, SIGNAL(si_stateChanged()), SLOT(sl_taskFinished()));
        return task;
    }

    if (dataFinished()) {
        setDone();
        output->setEnded();
    }

    if (pairedReadsInput) {
        const QString error = checkPairedReads();
        if (!error.isEmpty()) {
            return new FailTask(error);
        }
    }

    return NULL;
}

void KrakenClassifyWorker::cleanup() {
    datasetName.clear();
}

void KrakenClassifyWorker::sl_taskFinished() {
    KrakenClassifyTask *task = qobject_cast<KrakenClassifyTask *>(sender());
    if (!task->isFinished() || task->hasError() || task->isCanceled()) {
        return;
    }

    const QString rawClassificationUrl = task->getRawClassificationUrl();
    const QString translatedClassificationUrl = task->getTranslatedClassificationUrl();

    QVariantMap data;
    data[KrakenClassifyWorkerFactory::OUTPUT_REPORT_URL_SLOT_ID] = rawClassificationUrl;
    output->put(Message(output->getBusType(), data));

    output->put(Message(output->getBusType(), data));
    context->getMonitor()->addOutputFile(translatedClassificationUrl, getActor()->getId());
}

bool KrakenClassifyWorker::isReadyToRun() const {
    return input->hasMessage() && (!pairedReadsInput || pairedInput->hasMessage());
}

bool KrakenClassifyWorker::dataFinished() const {
    return input->isEnded() || (pairedReadsInput && pairedInput->isEnded());
}

QString KrakenClassifyWorker::checkPairedReads() const {
    CHECK(pairedReadsInput, "");
    if (input->isEnded() && (!pairedInput->isEnded() || pairedInput->hasMessage())) {
        return tr("Not enough downstream reads datasets");
    }
    if (pairedInput->isEnded() && (!input->isEnded() || input->hasMessage())) {
        return tr("Not enough upstream reads datasets");
    }
    return "";
}

KrakenClassifyTaskSettings KrakenClassifyWorker::getSettings() {
    KrakenClassifyTaskSettings settings;
    settings.databaseUrl = getValue<QString>(KrakenClassifyWorkerFactory::DATABASE_ATTR_ID);
    settings.quickOperation = getValue<bool>(KrakenClassifyWorkerFactory::QUICK_OPERATION_ATTR_ID);
    settings.minNumberOfHits = getValue<int>(KrakenClassifyWorkerFactory::MIN_HITS_NUMBER_ATTR_ID);
    settings.numberOfThreads = getValue<int>(KrakenClassifyWorkerFactory::THREADS_NUMBER_ATTR_ID);
    settings.preloadDatabase = getValue<bool>(KrakenClassifyWorkerFactory::PRELOAD_DATABASE_ATTR_ID);

    const Message message = getMessageAndSetupScriptValues(input);
    settings.readsUrl = message.getData().toString();

    if (pairedReadsInput) {
        const Message pairedMessage = getMessageAndSetupScriptValues(pairedInput);
        settings.pairedReadsUrl = pairedMessage.getData().toString();
    }

    return settings;
}

}   // namespace LocalWorkflow
}   // namespace U2
