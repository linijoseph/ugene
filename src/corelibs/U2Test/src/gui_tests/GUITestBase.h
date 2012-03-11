#ifndef _U2_GUI_TEST_BASE_H_
#define _U2_GUI_TEST_BASE_H_

#include <U2Core/global.h>
#include <U2Core/Task.h>
#include <U2Core/MultiTask.h>
#include <U2Core/GObject.h>
#include <U2Core/DNASequenceObject.h>
#include <U2Gui/MainWindow.h>
#include <U2View/ADVSingleSequenceWidget.h>

#include <QtGui>
#include "GUITest.h"

namespace U2 {

#define TESTNAME(className) QString(GUI_TEST_PREFIX) + #className

#define GUI_TEST_CLASS_DECLARATION(className) \
    class className : public GUIMultiTest { \
    public: \
        className::className() : GUIMultiTest(TESTNAME(className)){ addSubTests(); } \
    private: \
        virtual void addSubTests(); \
    };

#define GUI_TEST_CLASS_DEFINITION(className) \
    void className::addSubTests()

#define GUI_TEST_CLASS(className) \
    class className : public GUITest { \
    public: \
    className () : GUITest(TESTNAME(className)){} \
    protected: \
        virtual void execute(U2OpStatus &os); \
    };

typedef QMap<QString, GUITest*> GUITestMap;

class U2TEST_EXPORT GUITestBase {
public:
    enum TestType {NORMAL, ADDITIONAL} type;

    virtual ~GUITestBase();

    bool registerTest(GUITest *test, TestType testType = NORMAL);
    GUITest *getTest(const QString &name, TestType testType = NORMAL); // removes item from GUITestBase

    GUITests getTests(TestType testType = NORMAL);

    static const QString unnamedTestsPrefix;

private:
    GUITestMap tests;
    GUITestMap additional; // GUI checks additional to the launched checks

    GUITest *findTest(const QString &name, TestType testType);

    GUITestMap &getMap(TestType testType);

    QString getNextTestName(TestType testType);

    bool isNewTest(GUITest *test, TestType testType);
    void addTest(GUITest *test, TestType testType);

    QString nameUnnamedTest(GUITest* test, TestType testType);
};

}

#endif
