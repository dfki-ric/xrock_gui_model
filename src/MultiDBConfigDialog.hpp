/**
 * \file MultiDBConfigDialog.hpp
 * \author Malte Langosz and Team
 * \brief Allows user to configure MultiDB Backend.
 **/



#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
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
       explicit  MultiDBConfigDialog(const std::string &conf_file, XRockIOLibrary *ioLibrary);
        ~MultiDBConfigDialog();

        bool load_state();
    
    private:
        std::string config_filename;
        XRockIOLibrary *ioLibrary;
        QVBoxLayout *vLayout;
        QComboBox *cb_main_server_type;
        QLabel* lb_main_server_path_or_url;
        QLineEdit *tf_main_server_path;
        QLineEdit *tf_main_server_graph;
        
        struct BackendItem
        {
            QString name;
            QString type;
            QString url_or_path;
            QString graph;

            bool operator==(const BackendItem &other) const
            {
                if (&other == this)
                    return true;
                return name == other.name and
                       type == other.type and
                       url_or_path == other.url_or_path and 
                       graph == other.graph;
            }
        };
    
        std::vector<BackendItem> backends;
        QTableWidget *table_backends;
        QComboBox *cb_new_type;
        QLineEdit *tf_new_name;
        QLineEdit *tf_new_url_or_path;
        QLineEdit *tf_new_graph;
        QPushButton *btn_add_new, *btn_remove;
        QPushButton *btn_finish;
        QPushButton *btn_reset_to_default;

        void update_backends_widget();
        void closeEvent(QCloseEvent *e);
        

    private slots:
        void on_add_btn_clicked();
        void on_remove_btn_clicked();
        void on_finish_btn_clicked();
        void on_reset_to_default_btn_clicked();
        void on_main_server_backend_change(const QString &new_backend);
        void on_table_backends_cell_change(int row, int column);

    };
} // end of namespace xrock_gui_model

