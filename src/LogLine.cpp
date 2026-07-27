#include "LogLine.h"

#include <QHash>
#include <QRegularExpression>

namespace {

const QString kLibchat = QStringLiteral("libchat");
const QString kChatModule = QStringLiteral("chat-module");
const QString kChatUi = QStringLiteral("chat-ui");
const QString kDelivery = QStringLiteral("delivery");
// Everything the four named domains do not claim: the core, its other modules,
// and any line whose shape was not recognised at all.
const QString kHost = QStringLiteral("host");

LogLine::Level levelForWord(const QString& word, bool* recognised)
{
    static const QHash<QString, LogLine::Level> byWord = {
        {QStringLiteral("trace"), LogLine::Debug},
        {QStringLiteral("debug"), LogLine::Debug},
        {QStringLiteral("info"), LogLine::Info},
        // The pseudo-level a host stamps on a module's stdout, which carries no
        // severity of its own.
        {QStringLiteral("out"), LogLine::Info},
        {QStringLiteral("warn"), LogLine::Warning},
        {QStringLiteral("warning"), LogLine::Warning},
        {QStringLiteral("err"), LogLine::Error},
        {QStringLiteral("error"), LogLine::Error},
        {QStringLiteral("critical"), LogLine::Error},
        {QStringLiteral("fatal"), LogLine::Error},
        // The three-letter spellings chronicles uses, which is how the delivery
        // node states a severity.
        {QStringLiteral("trc"), LogLine::Debug},
        {QStringLiteral("dbg"), LogLine::Debug},
        {QStringLiteral("inf"), LogLine::Info},
        {QStringLiteral("ntc"), LogLine::Info},
        {QStringLiteral("wrn"), LogLine::Warning},
        {QStringLiteral("fat"), LogLine::Error},
    };

    const auto found = byWord.constFind(word.toLower());
    *recognised = found != byWord.constEnd();
    return *recognised ? found.value() : LogLine::Info;
}

// The domain a bracketed tag names, or empty when it names no module.
QString domainForTag(const QString& tag)
{
    if (tag == QLatin1String("chat_module"))
        return kChatModule;
    if (tag == QLatin1String("chat_ui"))
        return kChatUi;
    if (tag == QLatin1String("delivery_module"))
        return kDelivery;
    return {};
}

} // namespace

const QStringList& logDomainNames()
{
    static const QStringList domains = {kLibchat, kChatModule, kChatUi, kDelivery, kHost};
    return domains;
}

LogLine LogLine::parse(const QString& line)
{
    LogLine parsed;
    parsed.raw = line;
    parsed.domain = kHost;
    parsed.message = line;

    static const QRegularExpression stamp(
        QStringLiteral(R"(^\[\d{4}-\d{2}-\d{2} (\d{2}:\d{2}:\d{2}\.\d+)\]\s*)"));
    const QRegularExpressionMatch stamped = stamp.match(line);
    if (!stamped.hasMatch())
        return parsed;
    parsed.time = stamped.captured(1);

    // Whatever bracketed fields the writer put after the timestamp, in whatever
    // order: a level word claims the level, and the rest name who wrote the
    // line. Reading them positionally would break on any writer that carries
    // one field more or fewer. A writer may name several scopes at once
    // (`[chat_module] [LogosObject]`), so the field that names a module wins
    // over the innermost one, which belongs to no module on its own.
    static const QRegularExpression field(QStringLiteral(R"(\[([^\]]*)\]\s*)"));
    QString moduleTag;
    bool haveLevel = false;
    int at = stamped.capturedEnd(0);
    while (true) {
        const QRegularExpressionMatch next =
            field.match(line, at, QRegularExpression::NormalMatch,
                        QRegularExpression::AnchorAtOffsetMatchOption);
        if (!next.hasMatch())
            break;
        at = next.capturedEnd(0);

        const QString value = next.captured(1);
        if (value.isEmpty())
            continue;
        bool recognised = false;
        const Level asLevel = levelForWord(value, &recognised);
        if (recognised && !haveLevel) {
            parsed.level = asLevel;
            haveLevel = true;
            continue;
        }
        if (!domainForTag(value).isEmpty())
            moduleTag = value;
    }
    parsed.message = line.mid(at);
    const QString named = domainForTag(moduleTag);
    parsed.domain = named.isEmpty() ? kHost : named;

    // A module whose stdout the host relays states its own severity and time at
    // the head of the line. The severity is the event's own, where the host's
    // pseudo-level says only that the line arrived; the time is the same
    // instant the row already shows in its own column, so it goes.
    static const QRegularExpression chronicled(QStringLiteral(
        R"(^(TRC|DBG|INF|NTC|WRN|ERR|FAT) \d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+[+-]\d{2}:\d{2} )"));
    const QRegularExpressionMatch stated = chronicled.match(parsed.message);
    if (stated.hasMatch()) {
        bool statedLevel = false;
        parsed.level = levelForWord(stated.captured(1), &statedLevel);
        parsed.message = parsed.message.mid(stated.capturedEnd(0));
        return parsed;
    }

    // A module that names its own severity and target says more than the tag
    // does: the severity is the event's own, and the target names which crate
    // inside the module spoke.
    static const QRegularExpression targeted(
        QStringLiteral(R"(^(ERROR|WARNING|INFO|DEBUG|TRACE): ([A-Za-z0-9_]+(?:::[A-Za-z0-9_]+)*): )"));
    const QRegularExpressionMatch attributed = targeted.match(parsed.message);
    if (!attributed.hasMatch())
        return parsed;

    bool recognised = false;
    parsed.level = levelForWord(attributed.captured(1), &recognised);
    const QString crate = attributed.captured(2).section(QStringLiteral("::"), 0, 0);
    if (crate == QLatin1String("libchat") || crate == QLatin1String("logos_generic_chat"))
        parsed.domain = kLibchat;
    // The target stays on the message: it is the only place the submodule that
    // spoke is named.
    parsed.message = parsed.message.mid(attributed.capturedEnd(1) + 2);

    return parsed;
}
