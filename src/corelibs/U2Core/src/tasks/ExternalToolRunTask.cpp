/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2013 UniPro <ugene@unipro.ru>
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

#include "ExternalToolRunTask.h"

#include <U2Core/AnnotationTableObject.h>
#include <U2Core/AppContext.h>
#include <U2Core/AppSettings.h>
#include <U2Core/BaseDocumentFormats.h>
#include <U2Core/ExternalToolRegistry.h>
#include <U2Core/IOAdapter.h>
#include <U2Core/L10n.h>
#include <U2Core/Log.h>
#include <U2Core/SaveDocumentTask.h>
#include <U2Core/UserApplicationsSettings.h>
#include <U2Core/ScriptingToolRegistry.h>
#include <U2Core/U2SafePoints.h>

#include <QtCore/QString>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>

namespace U2 {

ExternalToolRunTask::ExternalToolRunTask(const QString& _toolName, const QStringList& _arguments, ExternalToolLogParser*  _logParser, const QString& _workingDirectory, const QStringList& additionalPaths)
: Task(_toolName + " run task", TaskFlag_None),
  arguments(_arguments),
  logParser(_logParser),
  toolName(_toolName),
  workingDirectory(_workingDirectory),
  writeOutputToFile(false)
{
    externalToolProcess = new QProcess();
    ExternalTool * tool = AppContext::getExternalToolRegistry()->getByName(toolName);
    if (tool == NULL) {
        setError(tr("Undefined tool: '%1'").arg(toolName));
        return;
    }
    if (tool->getPath().isEmpty()) {
        setError(tr("Path for '%1' tool not set").arg(toolName));
        return;
    }
    if (!tool->isValid()) {
//        setError(tr("'%1' tool located in '%2' not exists or not valid").arg(toolName).arg(tool->getPath()));
//        return;
    }
    program=tool->getPath();
    toolRunnerProgram = tool->getToolRunnerProgram();
    if (!toolRunnerProgram.isEmpty()){
        ScriptingToolRegistry* stregister = AppContext::getScriptingToolRegistry();
        SAFE_POINT_EXT(stregister != NULL, setError("No scripting tool registry"), );
        ScriptingTool* stool = stregister->getByName(toolRunnerProgram);
        if(!stool || stool->getPath().isEmpty()){
            setError(QString("The tool %1 that runs %2 is not installed. Please set the path of the tool in the External Tools settings").arg(toolRunnerProgram).arg(toolName));
            return;
        }
        //program = toolRunnerProgram + " " + program;
        arguments.prepend(program);
        program = stool->getPath();
    }

    processEnvironment = QProcessEnvironment::systemEnvironment();
    foreach (QString path, additionalPaths) {
        path = path + ":" + processEnvironment.value("PATH");
        processEnvironment.insert("PATH", path);
    }

    coreLog.trace("Creating run task for: " + toolName);
}

QProcess * ExternalToolRunTask::process() {
    return externalToolProcess;
}

ExternalToolRunTask::~ExternalToolRunTask(){
    delete externalToolProcess;
}

void ExternalToolRunTask::setOutputFile(const QString &url) {
    writeOutputToFile = true;
    outputUrl = url;
}

void ExternalToolRunTask::setOutputProcess(QProcess *proc) {
    CHECK(NULL != externalToolProcess, );
    externalToolProcess->setStandardOutputProcess(proc);
}

class Helper {
public:
    Helper(QProcess **proc) : proc(proc){}
    ~Helper() {
        (*proc)->closeReadChannel(QProcess::StandardError);
        (*proc)->closeReadChannel(QProcess::StandardOutput);
        (*proc)->closeWriteChannel();
        (*proc)->close();
        delete (*proc);
        (*proc) = NULL;
    }
private:
    QProcess **proc;
};

void ExternalToolRunTask::prepare(){
    if (hasError() || isCanceled()) {
        Helper helper(&externalToolProcess);
        return;
    }
}

void ExternalToolRunTask::run(){
    if (hasError() || isCanceled()) {
        return;
    }
    Helper helper(&externalToolProcess);
    externalToolProcess->setProcessEnvironment(processEnvironment);
    if (writeOutputToFile) {
        externalToolProcess->setStandardOutputFile(outputUrl);
    }
    ExternalToolRunTaskHelper* h = new ExternalToolRunTaskHelper(this);
    connect(externalToolProcess,SIGNAL(readyReadStandardOutput()), h, SLOT(sl_onReadyToReadLog()));
    connect(externalToolProcess,SIGNAL(readyReadStandardError()), h, SLOT(sl_onReadyToReadErrLog()));

    algoLog.details(tr("Launching %1 tool: %2 %3").arg(toolName).arg(program).arg(arguments.join(" ")));
    if (!workingDirectory.isEmpty()){
        externalToolProcess->setWorkingDirectory(workingDirectory);
        algoLog.details(tr("Working directory is \"%1\"").arg(externalToolProcess->workingDirectory()));
    }

    bool started = ExternalToolSupportUtils::startExternalProcess(externalToolProcess, program, arguments);

    if (!started){
        ExternalTool* tool = AppContext::getExternalToolRegistry()->getByName(toolName);
        if (tool->isValid()){
            stateInfo.setError(tr("Can not run %1 tool.").arg(toolName));
        } else {
            stateInfo.setError(tr("Can not run %1 tool. May be tool path '%2' not valid?")
                               .arg(toolName)
                               .arg(AppContext::getExternalToolRegistry()->getByName(toolName)->getPath()));
        }
        return;
    }
    while(!externalToolProcess->waitForFinished(1000)){
        if (isCanceled()) {
            cancelProcess();
        }
    }
    {
        int exitCode = externalToolProcess->exitCode();
        if(exitCode != EXIT_SUCCESS && !hasError()) {
            setError(tr("%1 tool exited with code %2").arg(toolName).arg(exitCode));
        } else {
            algoLog.details(tr("Tool %1 finished successfully").arg(toolName));
        }
    }
    delete h;
}
Task::ReportResult ExternalToolRunTask::report(){
    return ReportResult_Finished;
}
void ExternalToolRunTask::cancelProcess(){
    CHECK(NULL != externalToolProcess, );
    externalToolProcess->kill();
}

ExternalToolRunTaskHelper::ExternalToolRunTaskHelper(ExternalToolRunTask* t) 
: p(t)
{ 
    logData.resize(1000); 
}

void ExternalToolRunTaskHelper::sl_onReadyToReadLog(){
    QMutexLocker locker(&logMutex);
    assert(p->isRunning());
    CHECK(NULL != p->externalToolProcess, );
    if (p->externalToolProcess->readChannel() == QProcess::StandardError) {
        p->externalToolProcess->setReadChannel(QProcess::StandardOutput);
    }
    int numberReadChars = p->externalToolProcess->read(logData.data(), logData.size());
    while(numberReadChars > 0){
        //call log parser
        QString line = QString::fromLocal8Bit(logData.constData(), numberReadChars);
        p->logParser->parseOutput(line);
        numberReadChars = p->externalToolProcess->read(logData.data(), logData.size());
    }
    p->stateInfo.progress = p->logParser->getProgress();
}

void ExternalToolRunTaskHelper::sl_onReadyToReadErrLog(){
    QMutexLocker locker(&logMutex);
    assert(p->isRunning());
    CHECK(NULL != p->externalToolProcess, );
    if (p->externalToolProcess->readChannel() == QProcess::StandardOutput) {
        p->externalToolProcess->setReadChannel(QProcess::StandardError);
    }
    int numberReadChars = p->externalToolProcess->read(logData.data(), logData.size());
    while(numberReadChars > 0){
        //call log parser
        QString line = QString::fromLocal8Bit(logData.constData(), numberReadChars);
        p->logParser->parseErrOutput(line);
        numberReadChars = p->externalToolProcess->read(logData.data(), logData.size());
    }
    QString lastErr = p->logParser->getLastError();
    if (!lastErr.isEmpty()) {
        p->stateInfo.setError(lastErr);
    }
    p->stateInfo.progress = p->logParser->getProgress();
}

////////////////////////////////////////
//ExternalToolLogParser
ExternalToolLogParser::ExternalToolLogParser() {
    progress=-1;
    lastLine="";
    lastErrLine="";
    lastError="";
}
void ExternalToolLogParser::parseOutput(const QString& partOfLog){
    lastPartOfLog=partOfLog.split(QChar('\n'));
    lastPartOfLog.first()=lastLine+lastPartOfLog.first();
    lastLine=lastPartOfLog.takeLast();
    foreach(QString buf, lastPartOfLog){
        if(buf.contains("error",Qt::CaseInsensitive)){
            setLastError(buf);
        }else{
            ioLog.trace(buf);
        }
    }
}

void ExternalToolLogParser::parseErrOutput(const QString& partOfLog){
    lastPartOfLog=partOfLog.split(QChar('\n'));
    lastPartOfLog.first()=lastErrLine+lastPartOfLog.first();
    lastErrLine=lastPartOfLog.takeLast();
    foreach(const QString& buf, lastPartOfLog){
        if(buf.contains("error",Qt::CaseInsensitive)){
            setLastError(buf);
        }else{
            ioLog.trace(buf);
        }
    }
}

void ExternalToolLogParser::setLastError(const QString &value) {
    if (!value.isEmpty()) {
        ioLog.error(value);
    }
    lastError = value;
}

////////////////////////////////////////
//ExternalToolSupportUtils
void ExternalToolSupportUtils::removeTmpDir( const QString& tmpDirUrl, U2OpStatus& os) {
    if (tmpDirUrl.isEmpty()) {
        os.setError(tr("Can not remove temporary directory: path is empty."));
        return;
    }
    QDir tmpDir(tmpDirUrl);
    foreach(const QString& file, tmpDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries)){
        if (!tmpDir.remove(file)) {
            os.setError(tr("Can not remove files from temporary directory."));
            return;
        }
    }
    if (!tmpDir.rmdir(tmpDir.absolutePath())){
        os.setError(tr("Can not remove directory for temporary files."));
    }
}

QString ExternalToolSupportUtils::createTmpDir(const QString &prePath, const QString &domain, U2OpStatus &os) {
    int i = 0;
    while (true) {
        QString tmpDirName = QString("d_%1").arg(i);
        QString tmpDirPath = prePath + "/" + domain + "/" + tmpDirName;
        QDir tmpDir(tmpDirPath);

        if (!tmpDir.exists()) {
            if (!QDir().mkpath(tmpDirPath)) {
                os.setError(tr("Can not create directory for temporary files: %1").arg(tmpDirPath));
            } 
            return tmpDir.absolutePath();
        }
        i++;
    }
}

QString ExternalToolSupportUtils::createTmpDir(const QString& domain, U2OpStatus& os) {
    QString tmpDirPath = AppContext::getAppSettings()->getUserAppsSettings()->getCurrentProcessTemporaryDirPath();
    return createTmpDir(tmpDirPath, domain, os);
}

Document * ExternalToolSupportUtils::createAnnotationsDocument(const QString &filePath,
                             const DocumentFormatId &format,
                             const QList<SharedAnnotationData> &anns,
                             U2OpStatus &os) {
     Document *doc = NULL;
     { // create document
         DocumentFormat *f = AppContext::getDocumentFormatRegistry()->getFormatById(format);
         IOAdapterFactory *iof = AppContext::getIOAdapterRegistry()->getIOAdapterFactoryById(BaseIOAdapters::LOCAL_FILE);
         doc = f->createNewLoadedDocument(iof, filePath, os);
         CHECK_OP(os, NULL);
         doc->setDocumentOwnsDbiResources(false);
     }

     { // add annotations object
         AnnotationTableObject *aobj = new AnnotationTableObject("anns");
         foreach(const SharedAnnotationData& ann, anns) {
             aobj->addAnnotation(new Annotation(ann));
         }
         doc->addObject(aobj);
     }
     return doc;
}

void ExternalToolSupportUtils::appendExistingFile(const QString &path, QStringList &files) {
    GUrl url(path);
    if (QFile::exists(url.getURLString())) {
        files << url.getURLString();
    }
}

#define WIN_LAUNCH_CMD_COMMAND "cmd /C "
#define START_WAIT_MSEC 3000

bool ExternalToolSupportUtils::startExternalProcess(QProcess *process, const QString &program, const QStringList &arguments) {
    process->start(program, arguments);
    bool started = process->waitForStarted(START_WAIT_MSEC);

#ifdef Q_OS_WIN32
    if(!started) {
        QString execStr = WIN_LAUNCH_CMD_COMMAND + program;
        foreach (const QString arg, arguments) {
            execStr += " " + arg;
        }
        process->start(execStr);
        coreLog.trace(tr("Can't run an executable file \"%1\" as it is. Try to run it as a cmd line command: \"%2\"")
            .arg(program).arg(execStr));
        started = process->waitForStarted(START_WAIT_MSEC);
    }
#endif

    return started;
}

}//namespace
