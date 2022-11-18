/**
 * \file MultiDBConfigDialog.hpp

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

namespace xrock_gui_model
{

    class MultiDBConfigDialog : public QDialog
    {
        Q_OBJECT

    public:
       explicit  MultiDBConfigDialog(const std::string &conf_file);
        ~MultiDBConfigDialog();

        void load_config();
        /*
      main_server:
      type: Serverless
      path: modkom/component_db
    import_servers:
      - type: Serverless
        path: modkom/component_db
        graph: graph_test
      - type: Client
        url: http:://localhost:8183
        graph: graph_unit_test
    */

    private:
        std::string config_filename;

    private:
        QVBoxLayout *vLayout;
        QComboBox *cb_main_server_type;
        QLineEdit *tf_main_server_path;
        struct BackendItem
        {
            QString type;
            QString url_or_path;
            QString graph;
            bool operator==(const BackendItem &other) const
            {
                if (&other == this)
                    return true;
                return type == other.type && url_or_path == other.url_or_path && graph == other.graph;
            }
        };

        std::vector<BackendItem> backends;
        QTableWidget *table_backends;
        QComboBox *cb_new_type;
        QLineEdit *tf_new_url_or_path;
        QLineEdit *tf_new_graph;
        QPushButton *btn_add_new, *btn_remove;

        QPushButton *btn_finish;

        void update_backends_widget();

    private slots:
        void on_add_btn_clicked();
        void on_remove_btn_clicked();
        void on_finish_btn_clicked();

        //     {
        //   "main_server": {
        //     "type": "Serverless",
        //     "path": "modkom/component_db"
        //   },
        //   "import_servers": [
        //     {
        //       "type": "Client",
        //       "url": "http://localhost:8183/",
        //       "graph": "graph_unit_test"
        //     },
        //     {
        //       "type": "Serverless",
        //       "path": "database/atomic_component_db",
        //       "graph": "graph_unit_test"
        //     }
        //   ]
        // }

        /*

        static std::string lastDomain, lastFilter;

      public slots:
        void addModel();
        void modelClicked(const QModelIndex &index);
        void versionChanged(const QString &versionName);
        void updateFilter(const QString &filter);
        void changeDomain(const QString &domain);
        void urlClicked(const QUrl &);

      signals:
        void sigLoadComponent(std::string domain, std::string model, std::string version);
        void sigAddComponent(std::string domain, std::string model, std::string version);

      private:
        ModelLib *modelLib;
        bool load, ignoreUpdate;
        std::string selectedDomain;
        std::string selectedModel;
        std::string selectedVersion;
        std::vector<std::pair<std::string, std::string>> modelList;
        configmaps::ConfigMap indexMap;

        QListWidget *models;
        QLineEdit *filterPattern;
        QComboBox *domainSelect;
        QComboBox *versionSelect;
        QLabel *versionLabel;
        QWebView *doc;
        mars::config_map_gui::DataWidget *dw;
        */
    };
} // end of namespace xrock_gui_model

