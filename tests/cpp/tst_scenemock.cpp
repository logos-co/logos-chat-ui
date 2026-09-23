#include <QMetaProperty>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRemoteObjectReplica>
#include <QTest>

#include "rep_ChatBackend_replica.h"

// The scenes in tests/scenes render the view against a QML stand-in for the
// ChatBackend replica, which has to keep the .rep's surface: a scene that sets
// a property the app no longer has shows a view the app never reaches.
class TestSceneMock : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void declaresTheRepsProperties();
    void declaresTheRepsSlotsAndSignals();
    void declaresTheRepsEnumValues();

private:
    QObject* create(const QByteArray& qml);

    QQmlEngine m_engine;
    std::unique_ptr<QObject> m_mock;
};

namespace {

const QMetaObject& replica = ChatBackendReplica::staticMetaObject;
const QMetaObject& replicaBase = QRemoteObjectReplica::staticMetaObject;

// The type a QML property declares for the replica's: an enum is an int there,
// and a list of maps a var.
QByteArray qmlTypeName(const QMetaProperty& property)
{
    if (property.isEnumType())
        return "int";
    if (property.metaType() == QMetaType::fromType<QVariantList>())
        return "QVariant";
    return property.typeName();
}

QStringList propertiesOf(const QMetaObject& meta, const QMetaObject& base)
{
    QStringList out;
    for (int i = base.propertyCount(); i < meta.propertyCount(); ++i) {
        const QMetaProperty p = meta.property(i);
        out << QStringLiteral("%1: %2").arg(p.name(), qmlTypeName(p));
    }
    return out;
}

// Signals and invokables, less the notify signals of the type's own properties
// (QML declares those implicitly, without the value repc passes) and the
// replica's push<Property> setters (a QML property is assigned directly).
QStringList methodsOf(const QMetaObject& meta, const QMetaObject& base)
{
    QSet<int> notifiers;
    QSet<QByteArray> pushSetters;
    for (int i = base.propertyCount(); i < meta.propertyCount(); ++i) {
        const QMetaProperty p = meta.property(i);
        notifiers << p.notifySignalIndex();
        QByteArray name = p.name();
        name[0] = QChar::toUpper(name[0]);
        pushSetters << "push" + name;
    }
    QStringList out;
    for (int i = base.methodCount(); i < meta.methodCount(); ++i) {
        const QMetaMethod m = meta.method(i);
        if (notifiers.contains(i) || m.access() == QMetaMethod::Private)
            continue;
        if (m.methodType() == QMetaMethod::Signal)
            out << QStringLiteral("signal %1").arg(m.methodSignature());
        else if (!pushSetters.contains(m.name()))
            out << QStringLiteral("invokable %1").arg(m.methodSignature());
    }
    return out;
}

// Empty when both sides list the same, else each entry only one side has.
QString mismatches(const QStringList& mock, const QStringList& rep)
{
    QStringList out;
    for (const QString& entry : rep) {
        if (!mock.contains(entry))
            out << QStringLiteral("missing from the mock: ") + entry;
    }
    for (const QString& entry : mock) {
        if (!rep.contains(entry))
            out << QStringLiteral("not in the .rep: ") + entry;
    }
    return out.join(u'\n');
}

} // namespace

QObject* TestSceneMock::create(const QByteArray& qml)
{
    QQmlComponent component(&m_engine);
    component.setData(qml, QUrl());
    QObject* object = component.create();
    if (!object)
        qWarning().noquote() << component.errorString();
    return object;
}

void TestSceneMock::initTestCase()
{
    m_engine.addImportPath(QStringLiteral(SCENE_MOCKS_DIR));
    m_mock.reset(create("import Logos.ChatBackend\nChatBackend {}"));
    QVERIFY(m_mock);
}

void TestSceneMock::declaresTheRepsProperties()
{
    const QString diff = mismatches(propertiesOf(*m_mock->metaObject(), QObject::staticMetaObject),
                                    propertiesOf(replica, replicaBase));
    QVERIFY2(diff.isEmpty(), qPrintable(diff));
}

void TestSceneMock::declaresTheRepsSlotsAndSignals()
{
    const QString diff = mismatches(methodsOf(*m_mock->metaObject(), QObject::staticMetaObject),
                                    methodsOf(replica, replicaBase));
    QVERIFY2(diff.isEmpty(), qPrintable(diff));
}

// A QML-declared enum is not on the mock's metaobject, so each of the replica's
// keys is read back through QML, the way the view reads it.
void TestSceneMock::declaresTheRepsEnumValues()
{
    QStringList expected;
    QStringList lookups;
    for (int e = replicaBase.enumeratorCount(); e < replica.enumeratorCount(); ++e) {
        const QMetaEnum metaEnum = replica.enumerator(e);
        for (int k = 0; k < metaEnum.keyCount(); ++k) {
            const QString key = QString::fromLatin1(metaEnum.key(k));
            expected << QStringLiteral("%1=%2").arg(key).arg(metaEnum.value(k));
            lookups << QStringLiteral("\"%1=\" + ChatBackend.%1").arg(key);
        }
    }
    QVERIFY(!expected.isEmpty());

    std::unique_ptr<QObject> values(
        create("import QtQml\nimport Logos.ChatBackend\nQtObject { readonly property list<string> values: ["
               + lookups.join(u", ").toUtf8() + "] }"));
    QVERIFY(values);
    QCOMPARE(values->property("values").toStringList(), expected);
}

QTEST_GUILESS_MAIN(TestSceneMock)
#include "tst_scenemock.moc"
