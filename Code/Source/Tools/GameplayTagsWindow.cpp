/*
 * Copyright (c) 2026 Heathen Engineering Limited
 * Irish Registered Company #556277
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GameplayTagsWindow.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QKeyEvent>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QSet>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QUrl>
#include <QVBoxLayout>

namespace FoundationGameplayTags
{
    static constexpr const char* kGpTagsExt  = ".gptags";
    static constexpr int         kColName     = 0;
    static constexpr int         kColButtons  = 1;
    static constexpr int         kBtnColWidth = 76;
    static constexpr int         kRoleFileIdx = Qt::UserRole + 0;
    static constexpr int         kRoleFullPath = Qt::UserRole + 1;

    // =========================================================================
    // Static helper — used by editor system component on Activate
    // =========================================================================

    QStringList GameplayTagsWindow::LoadAllProjectTags()
    {
        QStringList result;

        AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        if (projectPath.empty())
            return result;

        // QDirIterator on the real path avoids @projectroot@ aliases
        QStringList gptFiles;
        QDirIterator scanIt(QString::fromUtf8(projectPath.c_str()),
            QStringList() << "*.gptags", QDir::Files, QDirIterator::Subdirectories);
        while (scanIt.hasNext())
            gptFiles.append(scanIt.next());

        for (const QString& fp : gptFiles)
        {
            QFile f(fp);
            if (!f.open(QIODevice::ReadOnly))
                continue;
            QJsonParseError perr;
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
            f.close();
            if (doc.isNull() || !doc.isObject())
                continue;
            const QJsonObject root = doc.object();
            // Only include tags from files explicitly flagged as registered.
            if (!root["registered"].toBool(false))
                continue;
            const QJsonArray tags = root["tags"].toArray();
            for (const QJsonValue& tv : tags)
            {
                QString t = tv.toString().trimmed();
                if (!t.isEmpty() && !result.contains(t))
                    result.append(t);
            }
        }

        result.sort();
        return result;
    }

    // =========================================================================
    // Constructor
    // =========================================================================

    GameplayTagsWindow::GameplayTagsWindow(QWidget* parent)
        : QWidget(parent)
    {
        setWindowTitle("Gameplay Tags");

        // ── Toolbar ────────────────────────────────────────────────────────
        auto* toolbar  = new QWidget(this);
        auto* tbLayout = new QHBoxLayout(toolbar);
        tbLayout->setContentsMargins(4, 4, 4, 4);
        tbLayout->setSpacing(6);

        m_newFileBtn = new QPushButton("New File\u2026", toolbar);
        m_saveAllBtn = new QPushButton("Save All",       toolbar);
        m_openSrcBtn = new QPushButton("Open Source\u2026", toolbar);

        m_saveAllBtn->setEnabled(false);

        tbLayout->addWidget(m_newFileBtn);
        tbLayout->addWidget(m_saveAllBtn);
        tbLayout->addWidget(m_openSrcBtn);
        tbLayout->addStretch();

        // ── Tree ────────────────────────────────────────────────────────────
        m_tree = new QTreeWidget(this);
        m_tree->setColumnCount(2);
        m_tree->setHeaderHidden(true);
        m_tree->header()->setSectionResizeMode(kColName,    QHeaderView::Stretch);
        m_tree->header()->setSectionResizeMode(kColButtons, QHeaderView::Fixed);
        m_tree->header()->resizeSection(kColButtons, kBtnColWidth);
        m_tree->setUniformRowHeights(true);
        m_tree->setIndentation(16);
        m_tree->setAlternatingRowColors(false);
        // We handle editing manually via itemDoubleClicked so that we can
        // show the full dot-path rather than just the leaf name.
        m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tree->viewport()->installEventFilter(this);

        // ── Status bar ───────────────────────────────────────────────────────
        m_statusLabel = new QLabel("No .gptags files found.", this);

        // ── Main layout ──────────────────────────────────────────────────────
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(4, 4, 4, 4);
        mainLayout->setSpacing(4);
        mainLayout->addWidget(toolbar);
        mainLayout->addWidget(m_tree, 1);
        mainLayout->addWidget(m_statusLabel);

        // ── Connections ──────────────────────────────────────────────────────
        connect(m_newFileBtn, &QPushButton::clicked,
                this, &GameplayTagsWindow::OnNewFile);
        connect(m_saveAllBtn, &QPushButton::clicked,
                this, &GameplayTagsWindow::OnSaveAll);
        connect(m_openSrcBtn, &QPushButton::clicked,
                this, &GameplayTagsWindow::OnOpenSource);
        connect(m_tree, &QTreeWidget::itemChanged,
                this, &GameplayTagsWindow::OnItemChanged);

        // Double-click: start inline editing with the full dot-path
        connect(m_tree, &QTreeWidget::itemDoubleClicked,
            [this](QTreeWidgetItem* item, int col)
            {
                if (col != kColName)
                    return;
                const int     fileIdx  = item->data(kColName, kRoleFileIdx).toInt();
                const QString fullPath = item->data(kColName, kRoleFullPath).toString();
                // File headers have empty fullPath; skip them
                if (fileIdx < 0 || fullPath.isEmpty())
                    return;
                if (fileIdx >= m_loadedFiles.size())
                    return;
                // Only allow editing explicit tags (not virtual structural nodes)
                if (!m_loadedFiles[fileIdx].tags.contains(fullPath))
                    return;

                m_editingOldTag  = fullPath;
                m_editingFileIdx = fileIdx;

                // Temporarily replace the leaf name with the full dot-path so
                // the user sees and edits the whole path in the inline editor.
                m_tree->blockSignals(true);
                item->setText(kColName, fullPath);
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                m_tree->blockSignals(false);
                m_tree->editItem(item, kColName);
            });

        // When the inline editor closes (committed or canceled) always rebuild so
        // items revert to showing just the leaf segment rather than the full path.
        // itemChanged only fires when the text actually changed, so this handles
        // the case where the user commits without typing (no-change commit).
        connect(m_tree->itemDelegate(), &QAbstractItemDelegate::closeEditor,
            [this](QWidget*, QAbstractItemDelegate::EndEditHint)
            {
                if (!m_editingOldTag.isEmpty())
                {
                    // OnItemChanged didn't fire (text unchanged) — just restore
                    m_editingOldTag.clear();
                    m_editingFileIdx = -1;
                    RebuildTree();
                }
            });

        // ── File watcher ─────────────────────────────────────────────────────
        // Watches all .gptags files so that external writes (e.g. OghamStoryteller
        // injecting a new tag via AddTagToGptagsFile) trigger a live refresh.
        m_fileWatcher   = new QFileSystemWatcher(this);
        m_watchDebounce = new QTimer(this);
        m_watchDebounce->setSingleShot(true);
        m_watchDebounce->setInterval(300);
        connect(m_fileWatcher,   &QFileSystemWatcher::fileChanged,
                m_watchDebounce, QOverload<>::of(&QTimer::start));
        connect(m_watchDebounce, &QTimer::timeout,
                this,            &GameplayTagsWindow::OnFileWatchDebounce);
    }

    // =========================================================================
    // showEvent — rescan and rebuild each time the panel becomes visible
    // =========================================================================

    void GameplayTagsWindow::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        ScanAndLoadAll();
        RebuildTree();
    }

    // =========================================================================
    // eventFilter — cancel inline edit on Escape
    // =========================================================================

    bool GameplayTagsWindow::eventFilter(QObject* obj, QEvent* event)
    {
        if (obj == m_tree->viewport() && event->type() == QEvent::KeyPress)
        {
            auto* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Escape && !m_editingOldTag.isEmpty())
            {
                m_editingOldTag.clear();
                m_editingFileIdx = -1;
                RebuildTree();
                return true;
            }
        }
        return QWidget::eventFilter(obj, event);
    }

    // =========================================================================
    // Toolbar slots
    // =========================================================================

    void GameplayTagsWindow::OnNewFile()
    {
        // Default to project Assets folder
        QString defaultDir;
        AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        if (!projectPath.empty())
        {
            defaultDir = QString::fromUtf8(projectPath.c_str()) + "/Assets";
            QDir().mkpath(defaultDir);
        }
        if (defaultDir.isEmpty())
            defaultDir = QDir::homePath();

        QString path = QFileDialog::getSaveFileName(this,
            "New Gameplay Tags File", defaultDir,
            "Gameplay Tags (*.gptags)");
        if (path.isEmpty())
            return;
        if (!path.endsWith(kGpTagsExt))
            path += kGpTagsExt;

        {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly))
            {
                QMessageBox::critical(this, "Error",
                    QString("Cannot create: %1").arg(path));
                return;
            }
            QJsonObject root;
            root["tags"] = QJsonArray();
            file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        }

        // Check if somehow already present (re-create edge-case)
        for (const auto& lf : m_loadedFiles)
            if (lf.path == path)
            { RebuildTree(); return; }

        LoadedTagFile lf;
        lf.path  = path;
        lf.dirty = false;
        m_loadedFiles.append(lf);

        RebuildTree();
    }

    void GameplayTagsWindow::OnSaveAll()
    {
        for (int i = 0; i < m_loadedFiles.size(); ++i)
        {
            if (m_loadedFiles[i].dirty)
                SaveFile(i);
        }
        RebuildTree();
    }

    void GameplayTagsWindow::OnOpenSource()
    {
        QString defaultDir;
        AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        if (!projectPath.empty())
            defaultDir = QString::fromUtf8(projectPath.c_str());
        if (defaultDir.isEmpty())
            defaultDir = QDir::homePath();

        QString path = QFileDialog::getOpenFileName(this,
            "Open Gameplay Tags File", defaultDir,
            "Gameplay Tags (*.gptags)");
        if (path.isEmpty())
            return;

        // Skip if already loaded
        for (const auto& lf : m_loadedFiles)
            if (lf.path == path)
                return;

        LoadedTagFile lf;
        lf.path  = path;
        lf.dirty = false;
        if (!ParseFile(path, lf))
        {
            QMessageBox::critical(this, "Error",
                QString("Failed to parse: %1").arg(path));
            return;
        }
        m_loadedFiles.append(lf);
        RebuildTree();
    }

    // =========================================================================
    // OnItemChanged — inline-edit committed
    // =========================================================================

    void GameplayTagsWindow::OnItemChanged(QTreeWidgetItem* item, int col)
    {
        if (col != kColName)
            return;
        if (m_editingOldTag.isEmpty() || m_editingFileIdx < 0)
            return;

        const QString newTag = item->text(kColName).trimmed();
        const QString oldTag = m_editingOldTag;
        const int     fidx   = m_editingFileIdx;

        m_editingOldTag.clear();
        m_editingFileIdx = -1;

        if (newTag == oldTag || newTag.isEmpty())
        {
            RebuildTree();
            return;
        }

        // Rename the tag and all descendants that share the same prefix
        auto& lf = m_loadedFiles[fidx];
        QStringList updated;
        bool changed = false;
        for (const QString& tag : lf.tags)
        {
            if (tag == oldTag)
            {
                updated.append(newTag);
                changed = true;
            }
            else if (tag.startsWith(oldTag + "."))
            {
                updated.append(newTag + tag.mid(oldTag.length()));
                changed = true;
            }
            else
            {
                updated.append(tag);
            }
        }

        if (changed)
        {
            lf.tags = updated;
            lf.tags.sort();
            SetFileDirty(fidx, true);
        }

        RebuildTree();
    }

    // =========================================================================
    // File I/O
    // =========================================================================

    void GameplayTagsWindow::ScanAndLoadAll()
    {
        AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        if (projectPath.empty())
            return;

        // QDirIterator on the real path avoids @projectroot@ aliases
        QStringList found;
        QDirIterator it(QString::fromUtf8(projectPath.c_str()),
            QStringList() << "*.gptags", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
            found.append(it.next());

        // Merge: preserve already-loaded files (and their dirty state); add new ones.
        QList<LoadedTagFile> merged;
        for (const QString& fp : found)
        {
            bool existing = false;
            for (const auto& lf : m_loadedFiles)
            {
                if (lf.path == fp)
                {
                    merged.append(lf);
                    existing = true;
                    break;
                }
            }
            if (!existing)
            {
                LoadedTagFile lf;
                lf.path = fp;
                ParseFile(fp, lf);
                merged.append(lf);
            }
        }
        m_loadedFiles = merged;

        // Update the file watcher to track all currently-loaded .gptags files.
        // Remove paths no longer present; add new ones.
        if (m_fileWatcher)
        {
            const QStringList watched = m_fileWatcher->files();
            for (const QString& old : watched)
                if (!found.contains(old))
                    m_fileWatcher->removePath(old);
            for (const QString& fp : found)
                if (!watched.contains(fp))
                    m_fileWatcher->addPath(fp);
        }
    }

    void GameplayTagsWindow::OnFileWatchDebounce()
    {
        // A watched .gptags file changed on disk — reload all files from scratch
        // (preserve dirty state) then redraw. The file watcher clears its path on
        // file replacement (some editors write-rename), so re-add all paths after.
        const QStringList paths = m_fileWatcher ? m_fileWatcher->files() : QStringList{};

        // Force a fresh parse for every loaded file
        for (auto& lf : m_loadedFiles)
        {
            if (!lf.dirty)
            {
                LoadedTagFile fresh;
                fresh.path = lf.path;
                if (ParseFile(lf.path, fresh))
                    lf = fresh;
            }
        }

        // Re-register paths (some editors atomically replace files, which removes
        // the path from the watcher)
        if (m_fileWatcher)
        {
            for (const auto& lf : m_loadedFiles)
                if (!m_fileWatcher->files().contains(lf.path))
                    m_fileWatcher->addPath(lf.path);
        }

        RebuildTree();
        UpdateStatusBar();
    }

    bool GameplayTagsWindow::ParseFile(const QString& path, LoadedTagFile& lf)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return false;

        QJsonParseError perr;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
        f.close();

        if (doc.isNull() || !doc.isObject())
            return false;

        const QJsonObject root = doc.object();
        lf.registered = root["registered"].toBool(false);

        const QJsonArray tags = root["tags"].toArray();
        for (const QJsonValue& tv : tags)
        {
            QString t = tv.toString().trimmed();
            if (!t.isEmpty())
                lf.tags.append(t);
        }
        lf.tags.sort();
        return true;
    }

    bool GameplayTagsWindow::SaveFile(int fileIdx)
    {
        if (fileIdx < 0 || fileIdx >= m_loadedFiles.size())
            return false;

        auto& lf = m_loadedFiles[fileIdx];

        QJsonArray tags;
        for (const QString& t : lf.tags)
            if (!t.isEmpty())
                tags.append(t);

        QJsonObject root;
        root["registered"] = lf.registered;
        root["tags"]       = tags;

        QFile f(lf.path);
        if (!f.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, "Save Error",
                QString("Cannot write to: %1").arg(lf.path));
            return false;
        }
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));

        lf.dirty = false;
        return true;
    }

    // =========================================================================
    // Tree building
    // =========================================================================

    void GameplayTagsWindow::RebuildTree()
    {
        // ── Save expansion state ──────────────────────────────────────────────
        // Key format: "<fileIdx>" for file-header rows, "<fileIdx>:<fullPath>" for tag nodes.
        // Collected before clear so the items still exist when we iterate.
        const bool firstBuild = (m_tree->topLevelItemCount() == 0);
        QSet<QString> expandedKeys;
        if (!firstBuild)
        {
            QTreeWidgetItemIterator it(m_tree);
            while (*it)
            {
                if ((*it)->isExpanded())
                {
                    const int     fi   = (*it)->data(kColName, kRoleFileIdx).toInt();
                    const QString path = (*it)->data(kColName, kRoleFullPath).toString();
                    expandedKeys.insert(path.isEmpty()
                        ? QString::number(fi)
                        : QString("%1:%2").arg(fi).arg(path));
                }
                ++it;
            }
        }

        m_tree->blockSignals(true);
        m_tree->clear();

        const QSet<QString> dups = FindDuplicates();

        for (int fileIdx = 0; fileIdx < m_loadedFiles.size(); ++fileIdx)
        {
            const auto& lf = m_loadedFiles[fileIdx];

            // ── File header ───────────────────────────────────────────────
            auto* fileItem = new QTreeWidgetItem(m_tree);
            const QString baseName = QFileInfo(lf.path).fileName();
            fileItem->setText(kColName, lf.dirty
                ? QString("\u25CF %1").arg(baseName)
                : baseName);
            fileItem->setData(kColName, kRoleFileIdx,  fileIdx);
            fileItem->setData(kColName, kRoleFullPath, QString());

            QFont boldFont = fileItem->font(kColName);
            boldFont.setBold(true);
            fileItem->setFont(kColName, boldFont);

            m_tree->setItemWidget(fileItem, kColButtons, MakeFileButtons(fileIdx));

            // ── Tag hierarchy ────────────────────────────────────────────
            // Build a set for O(1) lookup of explicitly listed tags
            QSet<QString> explicitSet;
            for (const QString& t : lf.tags)
                explicitSet.insert(t);

            // nodeMap tracks already-created tree items by full dot-path
            QMap<QString, QTreeWidgetItem*> nodeMap;

            for (const QString& tag : lf.tags)
            {
                const QStringList parts = tag.split('.', Qt::SkipEmptyParts);
                QTreeWidgetItem*  parent = fileItem;
                QString           curPath;

                for (const QString& seg : parts)
                {
                    if (!curPath.isEmpty())
                        curPath += '.';
                    curPath += seg;

                    if (!nodeMap.contains(curPath))
                    {
                        auto* node = new QTreeWidgetItem(parent);
                        node->setText(kColName, seg);
                        node->setData(kColName, kRoleFileIdx,  fileIdx);
                        node->setData(kColName, kRoleFullPath, curPath);

                        const bool isExplicit = explicitSet.contains(curPath);
                        const bool isDup      = dups.contains(curPath);

                        if (!isExplicit)
                        {
                            // Virtual structural node — italic, greyed out
                            QFont f = node->font(kColName);
                            f.setItalic(true);
                            node->setFont(kColName, f);
                            node->setForeground(kColName, QColor(128, 128, 128));
                        }
                        else if (isDup)
                        {
                            node->setForeground(kColName, QColor(200, 60, 60));
                        }

                        m_tree->setItemWidget(node, kColButtons,
                            MakeNodeButtons(fileIdx, curPath, isExplicit));

                        nodeMap.insert(curPath, node);
                        parent = node;
                    }
                    else
                    {
                        parent = nodeMap[curPath];
                    }
                }
            }

        }

        // ── Restore expansion state ───────────────────────────────────────────
        {
            QTreeWidgetItemIterator it(m_tree);
            while (*it)
            {
                const int     fi   = (*it)->data(kColName, kRoleFileIdx).toInt();
                const QString path = (*it)->data(kColName, kRoleFullPath).toString();
                const QString key  = path.isEmpty()
                    ? QString::number(fi)
                    : QString("%1:%2").arg(fi).arg(path);

                // On first build always expand file-header rows; otherwise restore saved state
                (*it)->setExpanded(firstBuild ? path.isEmpty() : expandedKeys.contains(key));
                ++it;
            }
        }

        m_tree->blockSignals(false);
        UpdateStatusBar();
    }

    QWidget* GameplayTagsWindow::MakeFileButtons(int fileIdx)
    {
        auto* w      = new QWidget;
        auto* layout = new QHBoxLayout(w);
        layout->setContentsMargins(2, 1, 2, 1);
        layout->setSpacing(2);

        // Registered toggle — left-aligned before the action buttons
        auto* regCheck = new QCheckBox("Register", w);
        regCheck->setChecked(m_loadedFiles[fileIdx].registered);
        regCheck->setToolTip(
            "When checked, this file's tags are compiled into a .tagbin product "
            "and treated as project-default tags at runtime.");
        layout->addWidget(regCheck);

        layout->addStretch();

        auto* addBtn  = new QPushButton("+",      w);
        auto* openBtn = new QPushButton("\u2026", w); // …
        addBtn->setFixedSize(22, 22);
        openBtn->setFixedSize(22, 22);
        addBtn->setToolTip("Add root tag to this file");
        openBtn->setToolTip("Reveal file in file browser");

        layout->addWidget(addBtn);
        layout->addWidget(openBtn);

        connect(regCheck, &QCheckBox::toggled,
                [this, fileIdx](bool checked)
                {
                    if (fileIdx < m_loadedFiles.size())
                    {
                        m_loadedFiles[fileIdx].registered = checked;
                        SetFileDirty(fileIdx, true);
                    }
                });
        connect(addBtn, &QPushButton::clicked,
                [this, fileIdx]() { AddRootTag(fileIdx); });
        connect(openBtn, &QPushButton::clicked,
                [this, fileIdx]()
                {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(
                        QFileInfo(m_loadedFiles[fileIdx].path).absolutePath()));
                });

        return w;
    }

    QWidget* GameplayTagsWindow::MakeNodeButtons(int fileIdx, const QString& fullPath, bool isExplicit)
    {
        auto* w      = new QWidget;
        auto* layout = new QHBoxLayout(w);
        layout->setContentsMargins(2, 1, 2, 1);
        layout->setSpacing(2);

        // Stretch first so buttons sit on the right edge of the column
        layout->addStretch();

        auto* addSibBtn = new QPushButton("+",           w);
        auto* addChiBtn = new QPushButton("\u2193",      w); // ↓
        addSibBtn->setFixedSize(22, 22);
        addChiBtn->setFixedSize(22, 22);
        addSibBtn->setToolTip("Add sibling tag");
        addChiBtn->setToolTip("Add child tag");

        layout->addWidget(addSibBtn);
        layout->addWidget(addChiBtn);

        if (isExplicit)
        {
            auto* remBtn = new QPushButton("X", w);
            remBtn->setFixedSize(22, 22);
            remBtn->setToolTip("Remove tag (and descendants)");
            remBtn->setStyleSheet("color: #cc3333; font-weight: bold;");
            layout->addWidget(remBtn);
            connect(remBtn, &QPushButton::clicked,
                    [this, fileIdx, fullPath]() { RemoveTag(fileIdx, fullPath); });
        }
        else
        {
            // Spacer to keep column width consistent with real nodes
            layout->addSpacing(24);
        }

        connect(addSibBtn, &QPushButton::clicked,
                [this, fileIdx, fullPath]() { AddSiblingTag(fileIdx, fullPath); });
        connect(addChiBtn, &QPushButton::clicked,
                [this, fileIdx, fullPath]() { AddChildTag(fileIdx, fullPath); });

        return w;
    }

    // =========================================================================
    // Tag CRUD
    // =========================================================================

    void GameplayTagsWindow::AddRootTag(int fileIdx)
    {
        if (fileIdx < 0 || fileIdx >= m_loadedFiles.size())
            return;

        const QString newTag = UniqueTag(fileIdx, "NewTag");
        m_loadedFiles[fileIdx].tags.append(newTag);
        m_loadedFiles[fileIdx].tags.sort();
        SetFileDirty(fileIdx, true);
        RebuildTree();
    }

    void GameplayTagsWindow::AddSiblingTag(int fileIdx, const QString& fullPath)
    {
        if (fileIdx < 0 || fileIdx >= m_loadedFiles.size())
            return;

        // Parent path = everything before the last "."
        const int     lastDot    = fullPath.lastIndexOf('.');
        const QString parentPath = (lastDot >= 0) ? fullPath.left(lastDot) : QString();
        const QString base       = parentPath.isEmpty() ? "NewTag" : parentPath + ".NewTag";
        const QString newTag     = UniqueTag(fileIdx, base);

        m_loadedFiles[fileIdx].tags.append(newTag);
        m_loadedFiles[fileIdx].tags.sort();
        SetFileDirty(fileIdx, true);
        RebuildTree();
    }

    void GameplayTagsWindow::AddChildTag(int fileIdx, const QString& fullPath)
    {
        if (fileIdx < 0 || fileIdx >= m_loadedFiles.size())
            return;

        const QString base   = fullPath + ".NewTag";
        const QString newTag = UniqueTag(fileIdx, base);

        m_loadedFiles[fileIdx].tags.append(newTag);
        m_loadedFiles[fileIdx].tags.sort();
        SetFileDirty(fileIdx, true);
        RebuildTree();
    }

    void GameplayTagsWindow::RemoveTag(int fileIdx, const QString& fullPath)
    {
        if (fileIdx < 0 || fileIdx >= m_loadedFiles.size())
            return;

        auto& lf = m_loadedFiles[fileIdx];

        // Collect tag itself and all descendants
        QStringList toRemove;
        for (const QString& tag : lf.tags)
        {
            if (tag == fullPath || tag.startsWith(fullPath + "."))
                toRemove.append(tag);
        }

        if (toRemove.isEmpty())
            return;

        if (toRemove.size() > 1)
        {
            const int r = QMessageBox::warning(this, "Remove Tag",
                QString("Removing '%1' will also remove %2 descendant tag(s):\n\n%3\n\nProceed?")
                    .arg(fullPath)
                    .arg(toRemove.size() - 1)
                    .arg(toRemove.join("\n")),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (r != QMessageBox::Yes)
                return;
        }

        for (const QString& t : toRemove)
            lf.tags.removeAll(t);

        SetFileDirty(fileIdx, true);
        RebuildTree();
    }

    // =========================================================================
    // Helpers
    // =========================================================================

    void GameplayTagsWindow::SetFileDirty(int fileIdx, bool dirty)
    {
        if (fileIdx < 0 || fileIdx >= m_loadedFiles.size())
            return;
        m_loadedFiles[fileIdx].dirty = dirty;
        UpdateStatusBar();
    }

    void GameplayTagsWindow::UpdateStatusBar()
    {
        int totalTags  = 0;
        int dirtyFiles = 0;
        for (const auto& lf : m_loadedFiles)
        {
            totalTags += lf.tags.size();
            if (lf.dirty)
                ++dirtyFiles;
        }

        QString msg = QString("%1 tag(s) in %2 file(s)")
            .arg(totalTags)
            .arg(m_loadedFiles.size());

        if (dirtyFiles > 0)
            msg += QString(" \u2014 %1 unsaved").arg(dirtyFiles); // —

        m_statusLabel->setText(msg);
        m_saveAllBtn->setEnabled(dirtyFiles > 0);
    }

    QSet<QString> GameplayTagsWindow::FindDuplicates() const
    {
        QMap<QString, int> counts;
        for (const auto& lf : m_loadedFiles)
            for (const QString& tag : lf.tags)
                counts[tag]++;

        QSet<QString> dups;
        for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
            if (it.value() > 1)
                dups.insert(it.key());
        return dups;
    }

    QString GameplayTagsWindow::UniqueTag(int fileIdx, const QString& base) const
    {
        const auto& tags = m_loadedFiles[fileIdx].tags;
        if (!tags.contains(base))
            return base;
        int n = 2;
        while (true)
        {
            const QString candidate = base + QString::number(n);
            if (!tags.contains(candidate))
                return candidate;
            ++n;
        }
    }

} // namespace FoundationGameplayTags

#include <moc_GameplayTagsWindow.cpp>
