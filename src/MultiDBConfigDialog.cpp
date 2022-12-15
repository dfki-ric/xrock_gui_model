#include "MultiDBConfigDialog.hpp"
#include <mars/config_map_gui/DataWidget.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QDesktopServices>
#include <array>
#include "DBInterface.hpp"
#include "XRockIOLibrary.hpp"
#include <configmaps/ConfigMap.hpp>

// Needed for version check
#include <QtGlobal>


using namespace configmaps;

namespace xrock_gui_model
{

    MultiDBConfigDialog::MultiDBConfigDialog(const std::string &conf_file, XRockIOLibrary *ioLibrary)
        : config_filename(conf_file), ioLibrary(ioLibrary)
    {
        this->setWindowTitle("MultiDB Configuration");
        QHBoxLayout *mainLayout = new QHBoxLayout();
        vLayout = new QVBoxLayout();

        QLabel *label = new QLabel("Main Server:");
        vLayout->addWidget(label);
        QHBoxLayout *hLayout = new QHBoxLayout();

        label = new QLabel("Type:");
        hLayout->addWidget(label);
        cb_main_server_type = new QComboBox();
        std::vector<std::string> backends;
        if(ioLibrary)
        {
            backends = ioLibrary->getBackends();
        }
        else
        {
            backends.push_back("FileDB");
        }

        for (std::string backend : backends)
        {
            if (backend != "MultiDbClient")
                cb_main_server_type->addItem(QString::fromStdString(backend));
        }
        hLayout->addWidget(cb_main_server_type);
        label = new QLabel("Path:");
        hLayout->addWidget(label);
        tf_main_server_path = new QLineEdit();
        hLayout->addWidget(tf_main_server_path);
        vLayout->addLayout(hLayout);

        label = new QLabel("Import Servers:");
        vLayout->addWidget(label);

        table_backends = new QTableWidget();
        table_backends->setColumnCount(3);
        table_backends->setHorizontalHeaderLabels(QStringList() << "Backend Type"
                                                                << "URL/Path"
                                                                << "Graph");
#if QT_VERSION >= 0x050000
        table_backends->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
        table_backends->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif
        table_backends->setSelectionBehavior(QAbstractItemView::SelectRows);

        vLayout->addWidget(table_backends);
        btn_remove = new QPushButton();
        btn_remove->setText("remove selected");
        connect(btn_remove, SIGNAL(clicked()), this, SLOT(on_remove_btn_clicked()));
        vLayout->addWidget(btn_remove);

        QHBoxLayout *hbox = new QHBoxLayout();

        cb_new_type = new QComboBox();
        for (std::string backend : backends)
        {
            if (backend != "MultiDbClient")
                cb_new_type->addItem(QString::fromStdString(backend));
        }
        tf_new_url_or_path = new QLineEdit();
        tf_new_url_or_path->setPlaceholderText("URL/Path");
        tf_new_graph = new QLineEdit();
        tf_new_graph->setPlaceholderText("Graph");

        btn_add_new = new QPushButton();
        btn_add_new->setText("add");
        connect(btn_add_new, SIGNAL(clicked()), this, SLOT(on_add_btn_clicked()));

        hbox->addWidget(cb_new_type);
        hbox->addWidget(tf_new_url_or_path);
        hbox->addWidget(tf_new_graph);
        hbox->addWidget(btn_add_new);
        vLayout->addLayout(hbox);

        btn_finish = new QPushButton("finish");
        connect(btn_finish, SIGNAL(clicked()), this, SLOT(on_finish_btn_clicked()));
        vLayout->addWidget(btn_finish);

        // if there is an existing config .yml load it to gui
        load_config();

        mainLayout->addLayout(vLayout);
        setLayout(mainLayout);
    }

    void MultiDBConfigDialog::load_config()
    {
        if (mars::utils::pathExists(config_filename))
        {
            std::cout << "config_filenmae: " << config_filename << std::endl;
            ConfigMap config = ConfigMap::fromYamlFile(config_filename);
            tf_main_server_path->setText(QString::fromStdString(config["main_server"]["path"]));
            for (auto backend : config["import_servers"])
            {
                BackendItem w;
                w.type = QString::fromStdString(backend["type"]);
                w.url_or_path = QString::fromStdString(backend["type"] == std::string("Client") ? backend["url"] : backend["path"]);
                w.graph = QString::fromStdString(backend["graph"]);
                backends.push_back(std::move(w));
            }
            update_backends_widget();
        }
    }
    void MultiDBConfigDialog::update_backends_widget()
    {
        table_backends->setRowCount(0);
        for (const BackendItem &w : backends)
        {
            int rowPosition = table_backends->rowCount();
            table_backends->insertRow(rowPosition);

            table_backends->setItem(rowPosition, 0, new QTableWidgetItem(w.type));
            table_backends->setItem(rowPosition, 1, new QTableWidgetItem(w.url_or_path));
            table_backends->setItem(rowPosition, 2, new QTableWidgetItem(w.graph));
        }
    }

    void MultiDBConfigDialog::on_remove_btn_clicked()
    {
        if (table_backends->selectionModel()->hasSelection())
        {

            auto index = table_backends->selectionModel()->currentIndex();

            BackendItem w;
            w.type = index.sibling(index.row(), 0).data().toString();
            w.url_or_path = index.sibling(index.row(), 1).data().toString();
            w.graph = index.sibling(index.row(), 2).data().toString();

            auto it = std::find(backends.begin(), backends.end(), w);
            if (it != backends.end())
            {
                backends.erase(it);
                update_backends_widget();
            }
        }
    }

    void MultiDBConfigDialog::on_add_btn_clicked()
    {
        if (tf_new_url_or_path->text().isEmpty())
        {
            QMessageBox::warning(this, "Warning", "URL/Path is empty!", QMessageBox::Ok);
            return;
        }
        if (tf_new_graph->text().isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Graph is empty!", QMessageBox::Ok);
            return;
        }
        BackendItem b;
        b.type = cb_new_type->currentText();
        b.url_or_path = tf_new_url_or_path->text();
        b.graph = tf_new_graph->text();

        // if backend already exists, remove it, otherwise, add it
        auto it = std::find(backends.begin(), backends.end(), b);
        if (it == backends.end()) // add
        {
            backends.push_back(std::move(b));
            update_backends_widget();
        }
    }

    void MultiDBConfigDialog::on_finish_btn_clicked()
    {
        QString main_server_type = cb_main_server_type->currentText();
        QString main_server_path = tf_main_server_path->text();
        if (main_server_path.isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Main server path/graph is empty!", QMessageBox::Ok);
            return;
        }
        if (backends.empty())
        {
            QMessageBox::warning(this, "Warning", "Import servers are empty!", QMessageBox::Ok);
            return;
        }

        ConfigMap config;
        config["main_server"]["type"] = main_server_type.toStdString();
        config["main_server"]["path"] = main_server_path.toStdString();

        for (const BackendItem &w : backends)
        {
            ConfigMap backend;
            backend["type"] = w.type.toStdString();
            if (backend["type"].getString() == "Client")
                backend["url"] = w.url_or_path.toStdString();
            else
                backend["path"] = w.url_or_path.toStdString();
            backend["graph"] = w.graph.toStdString();
            config["import_servers"].push_back(std::move(backend));
        }

        // user can click finish without adding servers
        std::cout << this->config_filename << std::endl;
        std::cout << config.toJsonString() << std::endl;
        config.toYamlFile(this->config_filename);
        done(0);
    }
    MultiDBConfigDialog::~MultiDBConfigDialog()
    {
    }

} // end of namespace xrock_gui_model
