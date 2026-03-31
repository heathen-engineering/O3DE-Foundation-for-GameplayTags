
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QWidget>
#endif

namespace FoundationGameplayTags
{
    class FoundationGameplayTagsWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit FoundationGameplayTagsWidget(QWidget* parent = nullptr);
    };
} 
