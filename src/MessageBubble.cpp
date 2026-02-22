#include "MessageBubble.h"
#include <QHBoxLayout>
#include <QFrame>

MessageBubble::MessageBubble(const QString& content, const QDateTime& timestamp, 
                             bool isMe, QWidget* parent)
    : QWidget(parent)
    , m_content(content)
    , m_timestamp(timestamp)
    , m_isMe(isMe)
    , m_contentLabel(nullptr)
    , m_timestampLabel(nullptr)
{
    setupUI();
}

void MessageBubble::setupUI()
{
    // Main horizontal layout for alignment
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 5, 10, 5);
    mainLayout->setSpacing(0);

    // Container frame for the bubble (QFrame works better with border-radius)
    QFrame* bubbleContainer = new QFrame(this);
    bubbleContainer->setFrameShape(QFrame::NoFrame);
    bubbleContainer->setAutoFillBackground(true);
    QVBoxLayout* bubbleLayout = new QVBoxLayout(bubbleContainer);
    bubbleLayout->setContentsMargins(16, 16, 16, 16);
    bubbleLayout->setSpacing(4);

    // Content label
    m_contentLabel = new QLabel(m_content, bubbleContainer);
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_contentLabel->setMinimumWidth(100);
    
    QFont contentFont = m_contentLabel->font();
    contentFont.setPointSize(13);
    m_contentLabel->setFont(contentFont);

    // Timestamp label
    m_timestampLabel = new QLabel(m_timestamp.toString("h:mm AP"), bubbleContainer);
    QFont timestampFont = m_timestampLabel->font();
    timestampFont.setPointSize(10);
    m_timestampLabel->setFont(timestampFont);

    bubbleLayout->addWidget(m_contentLabel);
    bubbleLayout->addWidget(m_timestampLabel);

    // Spacer widget for the empty half
    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    // Bubble container should expand but not beyond half
    bubbleContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // Style the bubble based on sender
    if (m_isMe) {
        // My message - right aligned, green background (terminal style)
        bubbleContainer->setStyleSheet(
            "QFrame {"
            "  background-color: #10B981;"
            "  border: none;"
            "  border-radius: 8px;"
            "}"
        );
        m_contentLabel->setStyleSheet("QLabel { color: #0A0A0A; background: transparent; border: none; }");
        m_timestampLabel->setStyleSheet("QLabel { color: rgba(10, 10, 10, 0.6); background: transparent; border: none; }");
        m_timestampLabel->setAlignment(Qt::AlignRight);
        // Spacer on left (50%), bubble on right (50%)
        mainLayout->addWidget(spacer, 1);
        mainLayout->addWidget(bubbleContainer, 1);
    } else {
        // Counterparty message - left aligned, dark bordered background
        bubbleContainer->setStyleSheet(
            "QFrame {"
            "  background-color: #1F1F1F;"
            "  border: 1px solid #2a2a2a;"
            "  border-radius: 8px;"
            "}"
        );
        m_contentLabel->setStyleSheet("QLabel { color: #FAFAFA; background: transparent; border: none; }");
        m_timestampLabel->setStyleSheet("QLabel { color: #4B5563; background: transparent; border: none; }");
        m_timestampLabel->setAlignment(Qt::AlignLeft);
        // Bubble on left (50%), spacer on right (50%)
        mainLayout->addWidget(bubbleContainer, 1);
        mainLayout->addWidget(spacer, 1);
    }

    // Set reasonable min/max widths for the bubble
    bubbleContainer->setMinimumWidth(120);
}
