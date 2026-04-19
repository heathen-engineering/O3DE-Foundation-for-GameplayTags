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

#pragma once

#if !defined(Q_MOC_RUN)
#include <QList>
#include <QSet>
#include <QStringList>
#include <QWidget>
#endif

class QFileSystemWatcher;
class QLabel;
class QPushButton;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;

namespace FoundationGameplayTags
{
    /// One loaded .gptags file with its flat sorted tag list and dirty flag.
    struct LoadedTagFile
    {
        QString     path;
        QStringList tags;       // sorted flat list of full dot-path tag strings
        bool        registered = false; // if true, builder compiles this into a .tagbin product
        bool        dirty      = false;
    };

    // =========================================================================
    // GameplayTagsWindow
    //
    // Multi-file editor for .gptags gameplay tag definition files.
    //
    // Layout:
    //   Toolbar  — [New File] [Save All] [Open Source…]
    //   ┌───────────────────────────────────────────────┬────────┐
    //   │  # tags.gptags                           [+][…]│        │
    //   │    Status                            [+][↓][-] │        │
    //   │      Burning                         [+][↓][-] │        │
    //   │      Frozen (virtual)                   [+][↓] │        │
    //   └───────────────────────────────────────────────┴────────┘
    //   Status bar
    //
    // Double-click a tag node to inline-edit its full dot-path.
    // Duplicate tags across files are coloured red.
    // Removing a node removes it and all descendants (warning shown first).
    // Virtual nodes (structural parents not explicitly in the file) appear
    // in italic/grey and cannot be removed but can be given children.
    // =========================================================================
    class GameplayTagsWindow : public QWidget
    {
        Q_OBJECT

    public:
        static const char* PanelName()    { return "Gameplay Tags"; }
        static const char* MenuCategory() { return "Heathen Tools"; }

        explicit GameplayTagsWindow(QWidget* parent = nullptr);

        /// Reads all .gptags files under the project root and returns every
        /// unique tag string found. Called by the editor system component on
        /// Activate to bulk-register tags with GameplayTagRegistry.
        static QStringList LoadAllProjectTags();

    protected:
        void showEvent(QShowEvent* event) override;
        bool eventFilter(QObject* obj, QEvent* event) override;

    private slots:
        void OnNewFile();
        void OnSaveAll();
        void OnOpenSource();
        void OnItemChanged(QTreeWidgetItem* item, int column);
        void OnFileWatchDebounce();

    private:
        // ── File I/O ─────────────────────────────────────────────────────────
        void ScanAndLoadAll();
        bool ParseFile(const QString& path, LoadedTagFile& lf);
        bool SaveFile(int fileIdx);

        // ── Tree building ────────────────────────────────────────────────────
        void     RebuildTree();
        QWidget* MakeFileButtons(int fileIdx);
        QWidget* MakeNodeButtons(int fileIdx, const QString& fullPath, bool isExplicit);

        // ── Tag CRUD ─────────────────────────────────────────────────────────
        void AddRootTag    (int fileIdx);
        void AddSiblingTag (int fileIdx, const QString& fullPath);
        void AddChildTag   (int fileIdx, const QString& fullPath);
        void RemoveTag     (int fileIdx, const QString& fullPath);

        // ── Helpers ──────────────────────────────────────────────────────────
        void          SetFileDirty    (int fileIdx, bool dirty);
        void          UpdateStatusBar ();
        QSet<QString> FindDuplicates  () const;
        QString       UniqueTag       (int fileIdx, const QString& base) const;

        // ── Toolbar ──────────────────────────────────────────────────────────
        QPushButton* m_newFileBtn  = nullptr;
        QPushButton* m_saveAllBtn  = nullptr;
        QPushButton* m_openSrcBtn  = nullptr;

        // ── Tree (2 columns: name | buttons) ─────────────────────────────────
        QTreeWidget* m_tree = nullptr;

        // ── Status ───────────────────────────────────────────────────────────
        QLabel* m_statusLabel = nullptr;

        // ── Data ─────────────────────────────────────────────────────────────
        QList<LoadedTagFile> m_loadedFiles;

        // ── File watcher ─────────────────────────────────────────────────────
        QFileSystemWatcher* m_fileWatcher   = nullptr;
        QTimer*             m_watchDebounce = nullptr;

        // ── Inline-edit state ────────────────────────────────────────────────
        QString m_editingOldTag;
        int     m_editingFileIdx = -1;
    };

} // namespace FoundationGameplayTags
