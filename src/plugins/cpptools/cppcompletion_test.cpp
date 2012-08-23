/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cpptoolsplugin.h"

#include <AST.h>
#include <Control.h>
#include <CppDocument.h>
#include <DiagnosticClient.h>
#include <Scope.h>
#include <TranslationUnit.h>
#include <Literals.h>
#include <Bind.h>
#include <Symbols.h>
#include <utils/changeset.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/plaintexteditor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/codeassist/iassistproposalmodel.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <cpptools/cpptoolsplugin.h>
#include <cpptools/cppcompletionassist.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/fileutils.h>

#include <QtTest>
#include <QDebug>
#include <QTextDocument>
#include <QDir>

/*!
    Tests for code completion.
 */
using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;
using namespace TextEditor;
using namespace Core;

struct TestData
{
    QByteArray srcText;
    int pos;
    Snapshot snapshot;
    BaseTextEditorWidget *editor;
    QTextDocument *doc;
};

static QStringList getCompletions(TestData &data)
{
    QStringList completions;

    CppCompletionAssistInterface *ai = new CppCompletionAssistInterface(data.editor->document(), data.pos,
                                                                        data.editor->editorDocument(), ExplicitlyInvoked,
                                                                        data.snapshot, QStringList(), QStringList());
    CppCompletionAssistProcessor processor;
    IAssistProposal *proposal = processor.perform(ai);
    if (!proposal)
        return completions;
    IAssistProposalModel *model = proposal->model();
    if (!model)
        return completions;
    BasicProposalItemListModel *listmodel = dynamic_cast<BasicProposalItemListModel *>(model);
    if (!listmodel)
        return completions;

    for (int i = 0; i < listmodel->size(); ++i)
        completions << listmodel->text(i);

    return completions;
}

static void setup(TestData *data)
{
    data->pos = data->srcText.indexOf('@');
    QVERIFY(data->pos != -1);
    data->srcText[data->pos] = ' ';
    Document::Ptr src = Document::create(QDir::tempPath() + QLatin1String("/file.h"));
    Utils::FileSaver srcSaver(src->fileName());
    srcSaver.write(data->srcText);
    srcSaver.finalize();
    src->setUtf8Source(data->srcText);
    src->parse();
    src->check();

    data->snapshot.insert(src);

    data->editor = new PlainTextEditorWidget(0);
    QString error;
    data->editor->open(&error, src->fileName(), src->fileName());

    data->doc = data->editor->document();
}

void CppToolsPlugin::test_completion_forward_declarations_present()
{
    TestData data;
    data.srcText = "\n"
            "class Foo\n"
            "{\n"
            "    struct Bar;\n"
            "    int i;\n"
            "};\n"
            "\n"
            "struct Foo::Bar \n"
            "{\n"
            "    Bar() {}\n"
            "};\n"
            "\n"
            "@\n"
            "// padding so we get the scope right\n";

    setup(&data);

    Utils::ChangeSet change;
    change.insert(data.pos, "Foo::Bar::");
    QTextCursor cursor(data.doc);
    change.apply(&cursor);
    data.pos += 10;

    QStringList expected;
    expected.append("Bar");

    QStringList completions = getCompletions(data);

    QCOMPARE(completions, expected);
}

void CppToolsPlugin::test_completion_basic_1()
{
    TestData data;
    data.srcText = "\n"
            "class Foo\n"
            "{\n"
            "    void foo();\n"
            "    int m;\n"
            "};\n"
            "\n"
            "void func() {\n"
            "    Foo f;\n"
            "    @\n"
            "    // padding so we get the scope right\n"
            "}";

    setup(&data);

    QStringList basicCompletions = getCompletions(data);

    QVERIFY(!basicCompletions.contains("foo"));
    QVERIFY(!basicCompletions.contains("m"));
    QVERIFY(basicCompletions.contains("Foo"));
    QVERIFY(basicCompletions.contains("func"));
    QVERIFY(basicCompletions.contains("f"));

    Utils::ChangeSet change;
    change.insert(data.pos, "f.");
    QTextCursor cursor(data.doc);
    change.apply(&cursor);
    data.pos += 2;

    QStringList memberCompletions = getCompletions(data);

    QVERIFY(memberCompletions.contains("foo"));
    QVERIFY(memberCompletions.contains("m"));
    QVERIFY(!memberCompletions.contains("func"));
    QVERIFY(!memberCompletions.contains("f"));
}

void CppToolsPlugin::test_completion_template_1()
{
    TestData data;
    data.srcText = "\n"
            "template <class T>\n"
            "class Foo\n"
            "{\n"
            "    typedef T Type;\n"
            "    T foo();\n"
            "    T m;\n"
            "};\n"
            "\n"
            "void func() {\n"
            "    Foo f;\n"
            "    @\n"
            "    // padding so we get the scope right\n"
            "}";

    setup(&data);

    Utils::ChangeSet change;
    change.insert(data.pos, "Foo::");
    QTextCursor cursor(data.doc);
    change.apply(&cursor);
    data.pos += 5;

    QStringList completions = getCompletions(data);

    QVERIFY(completions.contains("Type"));
    QVERIFY(completions.contains("foo"));
    QVERIFY(completions.contains("m"));
    QVERIFY(!completions.contains("T"));
    QVERIFY(!completions.contains("f"));
    QVERIFY(!completions.contains("func"));
}

void CppToolsPlugin::test_completion_template_as_base()
{
    QFETCH(QByteArray, code);
    QFETCH(QStringList, expectedCompletions);

    TestData data;
    data.srcText = code;
    setup(&data);

    Utils::ChangeSet change;
    change.insert(data.pos, "c.");
    QTextCursor cursor(data.doc);
    change.apply(&cursor);
    data.pos += 2;

    QStringList actualCompletions = getCompletions(data);
    actualCompletions.sort();
    expectedCompletions.sort();

    QCOMPARE(actualCompletions, expectedCompletions);
}

void CppToolsPlugin::test_completion_template_as_base_data()
{
    QTest::addColumn<QByteArray>("code");
    QTest::addColumn<QStringList>("expectedCompletions");

    QByteArray code;
    QStringList completions;

    code = "\n"
            "class Data { int dataMember; };\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "\n"
            "void func() {\n"
            "    Other<Data> c;\n"
            "    @\n"
            "    // padding so we get the scope right\n"
            "}";
    completions.append("Data");
    completions.append("dataMember");
    completions.append("Other");
    completions.append("otherMember");
    QTest::newRow("case: base as template directly") << code << completions;


    completions.clear();
    code = "\n"
            "class Data { int dataMember; };\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "template <class T> class More : public Other<T> { int moreMember; };\n"
            "\n"
            "void func() {\n"
            "    More<Data> c;\n"
            "    @\n"
            "    // padding so we get the scope right\n"
            "}";
    completions.append("Data");
    completions.append("dataMember");
    completions.append("Other");
    completions.append("otherMember");
    completions.append("More");
    completions.append("moreMember");
    QTest::newRow("case: base as class template") << code << completions;


    completions.clear();
    code = "\n"
            "class Data { int dataMember; };\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "template <class T> class More : public ::Other<T> { int moreMember; };\n"
            "\n"
            "void func() {\n"
            "    More<Data> c;\n"
            "    @\n"
            "    // padding so we get the scope right\n"
            "}";
    completions.append("Data");
    completions.append("dataMember");
    completions.append("Other");
    completions.append("otherMember");
    completions.append("More");
    completions.append("moreMember");
    QTest::newRow("case: base as globally qualified class template") << code << completions;


    completions.clear();
    code = "\n"
            "class Data { int dataMember; };\n"
            "namespace NS {\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "}\n"
            "template <class T> class More : public NS::Other<T> { int moreMember; };\n"
            "\n"
            "void func() {\n"
            "    More<Data> c;\n"
            "    @\n"
            "    // padding so we get the scope right\n"
            "}";
    completions.append("Data");
    completions.append("dataMember");
    completions.append("Other");
    completions.append("otherMember");
    completions.append("More");
    completions.append("moreMember");
    QTest::newRow("case: base as namespace qualified class template") << code << completions;


    completions.clear();
    code = "\n"
            "class Data { int dataMember; };\n"
            "namespace NS {\n"
            "template <class T> class Delegate { typedef Data<T> Type; };\n"
            "}\n"
            "template <class T> class Final : public NS::Delegate<T>::Type { int finalMember; };\n"
            "\n"
            "void func() {\n"
            "    Final<Data> c;\n"
            "    @\n"
            "    // padding so we get the scope right\n"
            "}";
    completions.append("Data");
    completions.append("dataMember");
    completions.append("Final");
    completions.append("finalMember");
    QTest::newRow("case: base as nested template name") << code << completions;


    completions.clear();
    code = "\n"
            "class Data { int dataMember; };\n"
            "namespace NS {\n"
            "template <class T> class Delegate { typedef Data<T> Type; };\n"
            "}\n"
            "class Final : public NS::Delegate<Data>::Type { int finalMember; };\n"
            "\n"
            "void func() {\n"
            "    Final c;\n"
            "    @\n"
            "    // padding so we get the scope right\n"
            "}";
    completions.append("Data");
    completions.append("dataMember");
    completions.append("Final");
    completions.append("finalMember");
    QTest::newRow("case: base as nested template name in non-template") << code << completions;

    completions.clear();
    code = "\n"
            "class Data { int dataMember; };\n"
            "namespace NS {\n"
            "template <class T> class Other : public T { int otherMember; };\n"
            "}\n"
            "class Final : public NS::Other<Data> { int finalMember; };\n"
            "\n"
            "void func() {\n"
            "    Final c;\n"
            "    @\n"
            "    // padding so we get the scope right\n"
            "}";
    completions.append("Data");
    completions.append("dataMember");
    completions.append("Final");
    completions.append("finalMember");
    completions.append("Other");
    completions.append("otherMember");
    QTest::newRow("case: base as template name in non-template") << code << completions;
}