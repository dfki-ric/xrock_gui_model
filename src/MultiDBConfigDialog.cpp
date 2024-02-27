
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

    MultiDBConfigDialog::MultiDBConfigDialog(const std::string &confFile, XRockIOLibrary *ioLibrary)
        : configFilename(confFile), ioLibrary(ioLibrary)
    {

        this->setWindowTitle("MultiDBClient Configuration");
        QVBoxLayout *mainLayout = new QVBoxLayout();
        std::vector<std::string> backendType;
        if (ioLibrary)
        {

            backendType = ioLibrary->getBackends();
        }
        else
        {
            backendType.push_back("FileDB");
        }
        //  Available Databases Label
        QLabel *labelAvailableDatabases = new QLabel("Available Databases:");
        labelAvailableDatabases->setStyleSheet("font-weight: bold; color: black;");
        mainLayout->addWidget(labelAvailableDatabases);

        // Table for displaying available databases
        tableBackends = new QTableWidget();
        tableBackends->setColumnCount(4); // Adjust column count as needed
        tableBackends->setHorizontalHeaderLabels(QStringList() << "Name"
                                                               << "Type"
                                                               << "URL/Path"
                                                               << "Graph");
#if QT_VERSION >= 0x050000
        tableBackends->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
        tableBackends->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif
        tableBackends->setSelectionBehavior(QAbstractItemView::SelectRows);
        connect(tableBackends, SIGNAL(cellChanged(int, int)), this, SLOT(onTableBackendsCellChange(int, int)));

        mainLayout->addWidget(tableBackends);

        QHBoxLayout *hbox = new QHBoxLayout();

        cbNewType = new QComboBox();
        for (std::string backend : backendType)
        {
            if (backend != "MultiDbClient")
                cbNewType->addItem(QString::fromStdString(backend));
        }

        tfNewName = new QLineEdit();
        tfNewName->setPlaceholderText("Name");
        tfNewUrlOrPath = new QLineEdit();
        tfNewUrlOrPath->setPlaceholderText("URL/Path");
        tfNewGraph = new QLineEdit();
        tfNewGraph->setPlaceholderText("Graph");

        btnAddNew = new QPushButton();
        btnAddNew->setText("add");
        connect(btnAddNew, SIGNAL(clicked()), this, SLOT(onAddBtnClicked()));
        hbox->addWidget(tfNewName);
        hbox->addWidget(cbNewType);
        hbox->addWidget(tfNewUrlOrPath);
        hbox->addWidget(tfNewGraph);
        hbox->addWidget(btnAddNew);
        mainLayout->addLayout(hbox);
        // remove
        btnRemove = new QPushButton();
        btnRemove->setText("remove selected");
        connect(btnRemove, SIGNAL(clicked()), this, SLOT(onRemoveBtnClicked()));
        mainLayout->addWidget(btnRemove);
 
        QFrame *separator = new QFrame;
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        mainLayout->addWidget(separator);

        //  combbox
        QHBoxLayout *hbx3 = new QHBoxLayout();
        QLabel *label = new QLabel("Main Server:");
        hbx3->addWidget(label);
        cbMainServer = new QComboBox();
        hbx3->addWidget(cbMainServer);
        mainLayout->addLayout(hbx3);

        QHBoxLayout *hbx4 = new QHBoxLayout();
        QLabel *lbLookupInMainDatabase = new QLabel("Also lookup in Main Server:");
        cbLookupInMainDatabase = new QCheckBox();
        hbx4->addWidget(lbLookupInMainDatabase);
        hbx4->addWidget(cbLookupInMainDatabase);
        mainLayout->addLayout(hbx4);
  
        separator = new QFrame;
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        mainLayout->addWidget(separator);

        // finish and save
        QHBoxLayout *hbx5 = new QHBoxLayout();
        btnResetToDefault = new QPushButton("reset to default");
        hbx5->addWidget(btnResetToDefault);
        connect(btnResetToDefault, SIGNAL(clicked()), this, SLOT(onResetToDefaultBtnClicked()));

        saveAndClose = new QPushButton("save and close");
        connect(saveAndClose, SIGNAL(clicked()), this, SLOT(onFinishBtnClicked()));
        hbx5->addWidget(saveAndClose);
        mainLayout->addLayout(hbx5);

        setLayout(mainLayout);
        // if there is an existing previous saved config state, load it to gui
        if (loadState())
        {
            std::cout << "Loaded previous MultiDbConfig state successfully" << std::endl;
        }
        else
        {
            // no previous saved state to load! load default config from selected bundle (if any)
            this->resetToDefault();
        }
    }

    bool MultiDBConfigDialog::loadState()
    {
        if (mars::utils::pathExists(configFilename))
        {
            ConfigMap config = ConfigMap::fromYamlFile(configFilename);

            backends.clear();
            BackendItem main_server;
            main_server.name = QString::fromStdString(config["main_server"].hasKey("name") ? config["main_server"]["name"] : std::string("server1"));
            main_server.type = QString::fromStdString(config["main_server"]["type"]);
            main_server.urlOrPath = QString::fromStdString(config["main_server"]["type"] == std::string("Client") ? config["main_server"]["url"] : config["main_server"]["path"]);
            main_server.graph = QString::fromStdString(config["main_server"]["graph"]);
            backends.push_back(std::move(main_server));
            int i = 2;
            for (auto &backend : config["import_servers"])
            {
                BackendItem w;
                w.name = QString::fromStdString(backend.hasKey("name") ? backend["name"] : std::string("server") + std::to_string(i++));
                w.type = QString::fromStdString(backend["type"]);
                w.urlOrPath = QString::fromStdString(backend["type"] == std::string("Client") ? backend["url"] : backend["path"]);
                w.graph = QString::fromStdString(backend["graph"]);
                backends.push_back(std::move(w));
            }

            updateBackendsWidget();
            updateSelectedMainServerCb();
            // set combobox current item to main_server
            cbMainServer->setCurrentIndex(cbMainServer->findData(main_server.name, Qt::DisplayRole));
            // set checkbox also_lookup_main_server value from state
            bool also_lookup_main_server = config.hasKey("also_lookup_main_server") ? config["also_lookup_main_server"] : false;
            cbLookupInMainDatabase->setChecked(also_lookup_main_server);
            return true;
        }
        else
        {
            return false;
        }
    }
    void MultiDBConfigDialog::updateBackendsWidget()
    {
        tableBackends->setRowCount(0);
        tableBackends->blockSignals(true);
        for (const BackendItem &w : backends)
        {
            int rowPosition = tableBackends->rowCount();
            tableBackends->insertRow(rowPosition);

            tableBackends->setItem(rowPosition, 0, new QTableWidgetItem(w.name));
            tableBackends->setItem(rowPosition, 1, new QTableWidgetItem(w.type));
            tableBackends->setItem(rowPosition, 2, new QTableWidgetItem(w.urlOrPath));
            tableBackends->setItem(rowPosition, 3, new QTableWidgetItem(w.graph));
        }
        tableBackends->blockSignals(false);
    }

    void MultiDBConfigDialog::onRemoveBtnClicked()
    {
        if (tableBackends->selectionModel()->hasSelection())
        {
            if (QMessageBox::question(this, "Warning", "Are you sure you want to delete the selected server?", QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
            {
                auto index = tableBackends->selectionModel()->currentIndex();

                BackendItem w;
                w.name = index.sibling(index.row(), 0).data().toString();
                w.type = index.sibling(index.row(), 1).data().toString();
                w.urlOrPath = index.sibling(index.row(), 2).data().toString();
                w.graph = index.sibling(index.row(), 3).data().toString();

                auto it = std::find(backends.begin(), backends.end(), w);
                if (it != backends.end())
                {
                    backends.erase(it);
                    updateBackendsWidget();
                    updateSelectedMainServerCb();
                }
            }
        }
    }

    void MultiDBConfigDialog::onAddBtnClicked()
    {
        if (tfNewName->text().isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Name is empty!", QMessageBox::Ok);
            return;
        }
        if (tfNewUrlOrPath->text().isEmpty())
        {
            QMessageBox::warning(this, "Warning", "URL/Path is empty!", QMessageBox::Ok);
            return;
        }
        if (tfNewGraph->text().isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Graph is empty!", QMessageBox::Ok);
            return;
        }
        BackendItem b;
        b.name = tfNewName->text();
        b.type = cbNewType->currentText();
        b.urlOrPath = tfNewUrlOrPath->text();
        b.graph = tfNewGraph->text();

        // if backend doesn't exists, add it
        auto it = std::find(backends.begin(), backends.end(), b);
        if (it == backends.end()) // add
        {
            backends.push_back(std::move(b));
            updateBackendsWidget();
            updateSelectedMainServerCb();
        }
        else
        {
            QMessageBox::warning(this, "Warning", "Import server \'" + b.name + "\' already exists.", QMessageBox::Ok);
        }
    }

    void MultiDBConfigDialog::resetToDefault()
    {
        if (ioLibrary)
        {
            auto defaultConfig = ioLibrary->getDefaultConfig();
            if (!defaultConfig.empty())
            {
                defaultConfig["MultiDbClient"].toYamlFile(this->configFilename);
                loadState();
            }
            else
            {
                QMessageBox::warning(this, "Warning", "Select bundle from where you want to load your default config.", QMessageBox::Ok);
            }
        }
    }
    void MultiDBConfigDialog::onResetToDefaultBtnClicked()
    {
        if (QMessageBox::question(this, "Warning",
                                  "Are you sure you want to reset the config to default?", QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes)
        {
            resetToDefault();
        }
    }

    void MultiDBConfigDialog::onFinishBtnClicked()
    {
        if (backends.empty())
        {
            QMessageBox::warning(this, "Warning", "Import servers are empty!", QMessageBox::Ok);
            return;
        }

        QString mainServerName = cbMainServer->currentText();
        const BackendItem &mainServer = *std::find_if(backends.begin(), backends.end(), [&](const BackendItem &b)
                                                      { return b.name == mainServerName; });
        ConfigMap config;
        config["main_server"]["type"] = mainServer.type.toStdString();
        config["main_server"]["graph"] = mainServer.graph.toStdString();
        config["main_server"]["name"] = mainServer.name.toStdString();
        std::string urlOrPath = config["main_server"]["type"] == "Client" ? "url" : "path";
        config["main_server"][urlOrPath] = mainServer.urlOrPath.toStdString();

        for (const BackendItem &b : backends)
        {
            // skip main server (already added to config above)
            if (b == mainServer)
                continue;
            ConfigMap backend;
            backend["name"] = b.name.toStdString();
            backend["type"] = b.type.toStdString();
            if (backend["type"].getString() == "Client")
                backend["url"] = b.urlOrPath.toStdString();
            else
                backend["path"] = b.urlOrPath.toStdString();
            backend["graph"] = b.graph.toStdString();
            config["import_servers"].push_back(std::move(backend));
        }
 
        config["also_lookup_main_server"] = cbLookupInMainDatabase->isChecked();
    
        // user can click finish without adding servers
        config.toYamlFile(this->configFilename);
        done(0);
    }

    void MultiDBConfigDialog::onTableBackendsCellChange(int row, int column)
    {
        switch (column)
        {
        case 0: // backend name
        {
            QString newName = tableBackends->item(row, column)->text();
            // don't allow empty names
            if (newName.trimmed().isEmpty())
            {
                QMessageBox::warning(this, "Warning", "Database unique name cannot be empty!", QMessageBox::Ok);
                // don't trigger another onTableBackendsCellChange
                tableBackends->blockSignals(true);
                tableBackends->item(row, column)->setText(backends[row].name);
                tableBackends->blockSignals(false);
                return;
            }
            // don't allow multiple names since it acts as a key for each backend
            if (std::any_of(backends.begin(), backends.end(), [&](const BackendItem &b)
                            { return b.name == newName; }))
            {
                // don't trigger another onTableBackendsCellChange
                tableBackends->blockSignals(true);
                tableBackends->item(row, column)->setText(backends[row].name);
                tableBackends->blockSignals(false);

                QMessageBox::warning(this, "Warning", "Database with name " + newName + " already exists! Please enter a unique name.", QMessageBox::Ok);
            }
            else
            {
                backends[row].name = newName;
                // new name, we should update selected main server combobox
                updateSelectedMainServerCb();
            }
            break;
        }
        case 1: // backend type
            backends[row].type = tableBackends->item(row, column)->text();
            break;
        case 2: // URL/path
            backends[row].urlOrPath = tableBackends->item(row, column)->text();
            break;
        case 3: // graph
            backends[row].graph = tableBackends->item(row, column)->text();
            break;
        default:
            throw std::runtime_error("Invalid table column");
        }
    }

    void MultiDBConfigDialog::updateSelectedMainServerCb()
    {
        cbMainServer->clear();

        for (const BackendItem &backend : backends)
            cbMainServer->addItem(backend.name);
    }

    void MultiDBConfigDialog::onMainServerBackendChange(const QString &newBackend)
    {
        if (newBackend == "Serverless")
        {
            lbMainServerPathOrUrl->setText("Path");
        }
        else if (newBackend == "Client")
        {
            lbMainServerPathOrUrl->setText("URL");
        }
    }
    void MultiDBConfigDialog::closeEvent(QCloseEvent *event)
    {
        int result = QMessageBox::question(this, tr("Confirm Close"), tr("Are you sure you want to close this window?"), QMessageBox::Yes | QMessageBox::No);

        if (result == QMessageBox::Yes)
        {
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }

    MultiDBConfigDialog::~MultiDBConfigDialog()
    {

        delete tableBackends;

        delete cbNewType;
        delete tfNewName;
        delete tfNewUrlOrPath;
        delete tfNewGraph;

        delete btnAddNew;
        delete btnRemove;
        delete saveAndClose;
        delete btnResetToDefault;
    }

} // end of namespace xrock_gui_model
