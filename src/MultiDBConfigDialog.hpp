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
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QVBoxLayout>
#include <QListView>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
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
            bool readOnly; // Add readOnly member
       

            bool operator==(const BackendItem &other) const
            {
                if (&other == this)
                    return true;
                return name == other.name and
                       type == other.type and
                       urlOrPath == other.urlOrPath and
                       graph == other.graph and
                       readOnly == other.readOnly;
            }
        };

        std::vector<BackendItem> backends;
        QTableWidget *tableBackends;
        QComboBox *cbNewType;
        QLineEdit *tfNewName;
        QLineEdit *tfNewUrlOrPath;
        QLineEdit *tfNewGraph;
        QCheckBox *cNewReadOnly;
        QPushButton *btnAddNew, *btnRemove;
        QPushButton *saveAndClose;
        QPushButton *btnResetToDefault;


        void updateBackendsWidget();
        void closeEvent(QCloseEvent *e);

        void updateTableCheckboxes(Qt::CheckState state, int excludeRow);
        bool areAllImportserversReadonly();
    private slots:
        void onMainserverReadOnlyCheckboxStateChange();
        void onAddBtnClicked();
        void onRemoveBtnClicked();
        void onFinishBtnClicked();
        void onResetToDefaultBtnClicked();
        void onMainServerBackendChange(const QString &newBackend);
        void onTableBackendsCellChange(int row, int column);
    };
} // end of namespace xrock_gui_model

