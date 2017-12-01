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

#include <U2Core/AppContext.h>
#include <U2Core/Counter.h>
#include <U2Core/U2SafePoints.h>

#include "KrakenSupport.h"
#include "KrakenClassifyTask.h"

namespace U2 {

KrakenClassifyTask::KrakenClassifyTask(const KrakenClassifyTaskSettings &settings)
    : ExternalToolSupportTask(tr("Classify reads with Kraken"), TaskFlags_FOSE_COSC),
      settings(settings)
{
    GCOUNTER(cvar, tvar, "KrakenClassifyTask");
}

const QString &KrakenClassifyTask::getReportUrl() const {
    return settings.reportUrl;
}

void KrakenClassifyTask::prepare() {
    QScopedPointer<ExternalToolRunTask> task(new ExternalToolRunTask(ET_KRAKEN_CLASSIFY, getArguments(), NULL));
    CHECK_OP(stateInfo, );
    setListenerForTask(task.data());
    addSubTask(task.take());
}

QStringList KrakenClassifyTask::getArguments() {
    // TODO: taxonomy is not processed

    QStringList arguments;
    arguments << "--db" << settings.databaseUrl;
    arguments << "--threads" << QString::number(settings.numberOfThreads);
    if (settings.quickOperation) {
        arguments << "--quick";
        arguments << "--min-hits" << QString::number(settings.minNumberOfHits);
    }
    arguments << "--output" << settings.reportUrl;
    if (settings.preloadDatabase) {
        arguments << "--preload";
    }
    if (!settings.pairedReadsUrls.isEmpty()) {
        CHECK_EXT(settings.readsUrls.size() == settings.pairedReadsUrls.size(), setError(tr("Direct reads URLs count doesn't fit reverse reads URLs count")), QStringList());
        arguments << "--paired";
    }
    arguments << "--check-names";

    // TODO: check, if there could be more than 1 URL in the list, if "paired" option is set
    // Add a check for 1 per list and do not add paired, if paired is turned off
    arguments << settings.readsUrls;
    arguments << settings.pairedReadsUrls;

    return arguments;
}

}   // namespace U2
