/**
 * \file MultiDBConfigDialog.hpp
 * \author Malte Langosz and Team
 * \brief Allows user to configure MultiDB Backend.
 **/

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QListView>
#include <QTableWidget>
#include <QHeaderView>
#include <mars/utils/misc.h>
#include "XRockIOLibrary.hpp"

namespace xrock_gui_model
{

    class MultiDBConfigDialog : public QDialog
    {
        Q_OBJECT

    public:
       explicit  MultiDBConfigDialog(const std::string &confFile, XRockIOLibrary *ioLibrary);
        ~MultiDBConfigDialog();

        bool loadState();
        void resetToDefault();

    private:
        std::string configFilename;
        XRockIOLibrary *ioLibrary;
        QVBoxLayout *vLayout;
        QComboBox *cbMainServerType;
        QLabel* lbMainServerPathOrUrl;
        QLineEdit *tfMainServerPath;
        QLineEdit *tfMainServerGraph;
        
        struct BackendItem
        {
            QString name;
            QString type;
            QString urlOrPath;
            QString graph;

            bool operator==(const BackendItem &other) const
            {
                if (&other == this)
                    return true;
                return name == other.name and
                       type == other.type and
                       urlOrPath == other.urlOrPath and 
                       graph == other.graph;
            }
        };

        std::vector<BackendItem> backends;
        BackendItem prevMainServer;
        QTableWidget *tableBackends;
        QComboBox *cbNewType;
        QComboBox *cbMainServer;
        QLineEdit *tfNewName;
        QLineEdit *tfNewUrlOrPath;
        QLineEdit *tfNewGraph;
        QPushButton *btnAddNew, *btnRemove;
        QCheckBox *cbLookupInMainDatabase;
        QPushButton *saveAndClose;
        QPushButton *btnResetToDefault;
        QPushButton *btnMoveUp;
        QPushButton *btnMoveDown;
        QPushButton *btnHelp;

        void updateBackendsWidget();
        void updateSelectedMainServerCb(); 
        void closeEvent(QCloseEvent *e) override;
        void keyPressEvent(QKeyEvent *event) override;
        void checkAndUpdateMainServerInImports();
        void updateMoveUpAndDownButtonState(int currentIndex);

    private slots:
        void onAddBtnClicked();
        void onRemoveBtnClicked();
        void onFinishBtnClicked();
        void onResetToDefaultBtnClicked();
        void onMainServerBackendChange(const QString &newBackend);
        void onTableBackendsCellChange(int row, int column);
        void onTableBackendsCellClick(int row, int column);
         void onMoveUpClicked();
        void onMoveDownClicked();
        void handleMainServerImport(int state);
        void updateMainServer();
        void onHelpButtonClicked();
        

    public slots:
        void highlightMainServer(const QString &mainServerName);
    };
} // end of namespace xrock_gui_model
