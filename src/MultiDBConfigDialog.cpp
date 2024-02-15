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
        this->setWindowTitle("MultiDB Configuration");
        QHBoxLayout *mainLayout = new QHBoxLayout();
        vLayout = new QVBoxLayout();

        QLabel *label = new QLabel("Main Server:");
        vLayout->addWidget(label);
        label->setStyleSheet("font-weight: bold; color: black;");
        QHBoxLayout *hLayout = new QHBoxLayout();

        label = new QLabel("Type:");
        hLayout->addWidget(label);
        cbMainServerType = new QComboBox();
    
        std::vector<std::string> backends;
        if (ioLibrary)
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
                cbMainServerType->addItem(QString::fromStdString(backend));
        }
        hLayout->addWidget(cbMainServerType);
        this->lbMainServerPathOrUrl = new QLabel("Path:");
        hLayout->addWidget(this->lbMainServerPathOrUrl);
        connect(cbMainServerType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(onMainServerBackendChange(const QString &)));
        tfMainServerPath = new QLineEdit();
        hLayout->addWidget(tfMainServerPath);
        tfMainServerGraph = new QLineEdit();
        QLabel *l_graph = new QLabel("Graph:");
        hLayout->addWidget(l_graph);
        hLayout->addWidget(tfMainServerGraph);
       
        cNewReadOnly = new QCheckBox("Read Only");
        cNewReadOnly->setLayoutDirection(Qt::RightToLeft);
    
        hLayout->addWidget(cNewReadOnly);
        vLayout->addLayout(hLayout);
   
        label = new QLabel("Import Servers:");
        label->setStyleSheet("font-weight: bold; color: black;");
        vLayout->addWidget(label);

        tableBackends = new QTableWidget();
        tableBackends->setColumnCount(5);
        tableBackends->setHorizontalHeaderLabels(QStringList() << "Name"
                                                                << "Backend Type"
                                                                << "URL/Path"
                                                                << "Graph"
                                                                << "Read Only");
#if QT_VERSION >= 0x050000
        tableBackends->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
        tableBackends->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif
        tableBackends->setSelectionBehavior(QAbstractItemView::SelectRows);
        connect(tableBackends, SIGNAL(cellChanged(int, int)), this, SLOT(onTableBackendsCellChange(int, int)));

        vLayout->addWidget(tableBackends);
        btnRemove = new QPushButton();
        btnRemove->setText("remove selected");
        connect(btnRemove, SIGNAL(clicked()), this, SLOT(onRemoveBtnClicked()));
        vLayout->addWidget(btnRemove);

        QHBoxLayout *hbox = new QHBoxLayout();
        QHBoxLayout *hbox2 = new QHBoxLayout();

        cbNewType = new QComboBox();
        for (std::string backend : backends)
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
        vLayout->addLayout(hbox);

        btnResetToDefault = new QPushButton("reset to default");
        hbox2->addWidget(btnResetToDefault);
        connect(btnResetToDefault, SIGNAL(clicked()), this, SLOT(onResetToDefaultBtnClicked()));
  
        saveAndClose = new QPushButton("save and close");
        connect(saveAndClose, SIGNAL(clicked()), this, SLOT(onFinishBtnClicked()));
        hbox2->addWidget(saveAndClose);

        vLayout->addLayout(hbox2);
        
        mainLayout->addLayout(vLayout);
        setLayout(mainLayout);
        connect(cNewReadOnly, SIGNAL(stateChanged(int)), this, SLOT(onMainserverReadOnlyCheckboxStateChange()));

        // if there is an existing previous saved config state, load it to gui
        if(loadState())
        {
            std::cout << "Loaded previous MultiDbConfig state successfully" << std::endl;
        }
        else {
            // no previous saved state to load! load default config from selected bundle (if any)
            this->resetToDefault();
        }
    }

    bool MultiDBConfigDialog::loadState()
    {
        if (mars::utils::pathExists(configFilename))
        {
            ConfigMap config = ConfigMap::fromYamlFile(configFilename);
            cbMainServerType->setCurrentIndex(cbMainServerType->findText(QString::fromStdString(config["main_server"]["type"]), Qt::MatchFixedString));
            tfMainServerPath->setText(QString::fromStdString(config["main_server"]["type"] == "Client" ? config["main_server"]["url"] : config["main_server"]["path"]));
            tfMainServerGraph->setText(QString::fromStdString(config["main_server"]["graph"]));
            lbMainServerPathOrUrl->setText(config["main_server"]["type"] == "Client" ? "URL" : "Path");
            cNewReadOnly->blockSignals(true);
            if (config["main_server"].hasKey("readonly"))

                cNewReadOnly->setCheckState(config["main_server"]["readonly"] ? Qt::Checked : Qt::Unchecked);
            else
                cNewReadOnly->setCheckState(Qt::Unchecked);
            cNewReadOnly->blockSignals(false);
            backends.clear();
            for (auto &backend : config["import_servers"])
            {
                BackendItem w;
                w.name = QString::fromStdString(backend["name"]);
                w.type = QString::fromStdString(backend["type"]);
                w.urlOrPath = QString::fromStdString(backend["type"] == std::string("Client") ? backend["url"] : backend["path"]);
                w.graph = QString::fromStdString(backend["graph"]);
                if(backend.hasKey("readonly"))
                    w.readOnly = backend["readonly"];
                else
                    w.readOnly = true;
                backends.push_back(std::move(w));
            }
            updateBackendsWidget();
            return true;
        }
        else {
            return false;
        }
    }
    void MultiDBConfigDialog::updateBackendsWidget()
    {
        tableBackends->setRowCount(0);
        for (const BackendItem &w : backends)
        {
            int rowPosition = tableBackends->rowCount();
            tableBackends->insertRow(rowPosition);

            tableBackends->setItem(rowPosition, 0, new QTableWidgetItem(w.name));
            tableBackends->setItem(rowPosition, 1, new QTableWidgetItem(w.type));
            tableBackends->setItem(rowPosition, 2, new QTableWidgetItem(w.urlOrPath));
            tableBackends->setItem(rowPosition, 3, new QTableWidgetItem(w.graph));

            // Add readOnly checkbox
            QTableWidgetItem *readOnlyItem = new QTableWidgetItem();
            readOnlyItem->setFlags(readOnlyItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
            readOnlyItem->setFlags(readOnlyItem->flags() & ~ Qt::ItemIsEditable);
            readOnlyItem->setCheckState(w.readOnly ? Qt::Checked : Qt::Unchecked);
            tableBackends->setItem(rowPosition, 4, readOnlyItem);
           
        }
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
                w.readOnly = index.sibling(index.row(), 4).data().toBool();

                 auto it = std::find_if(backends.begin(), backends.end(), [w](const BackendItem& item) {
                    return item.name == w.name && item.type == w.type && item.urlOrPath == w.urlOrPath && item.graph == w.graph;
                });
                if (it != backends.end())
                {
                    backends.erase(it);
                    updateBackendsWidget();
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
        b.readOnly = true;

        // if backend doesn't exists, add it
        auto it = std::find(backends.begin(), backends.end(), b);
        if (it == backends.end()) // add
        {
            backends.push_back(std::move(b));
            updateBackendsWidget();
        }
        else
        {
            QMessageBox::warning(this, "Warning", "Import server \'" + b.name + "\' already exists.", QMessageBox::Ok);
        }
    }

    void MultiDBConfigDialog::resetToDefault(){
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
        QString main_server_type = cbMainServerType->currentText();
        QString main_server_path = tfMainServerPath->text();
        QString main_server_graph = tfMainServerGraph->text();

        if (main_server_path.isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Main server path is empty!", QMessageBox::Ok);
            return;
        }
        if (main_server_graph.isEmpty())
        {
            QMessageBox::warning(this, "Warning", "Main server graph is empty!", QMessageBox::Ok);
            return;
        }
        if (backends.empty())
        {
            QMessageBox::warning(this, "Warning", "Import servers are empty!", QMessageBox::Ok);
            return;
        }
      
        ConfigMap config;
        config["main_server"]["type"] = main_server_type.toStdString();
        config["main_server"]["graph"] = main_server_graph.toStdString();
        std::string urlOrPath = config["main_server"]["type"] == "Client" ? "url" : "path";
        config["main_server"][urlOrPath] = main_server_path.toStdString();
        config["main_server"]["readonly"] = cNewReadOnly->checkState() == Qt::Checked;

        for (const BackendItem &w : backends)
        {
            ConfigMap backend;
            backend["name"] = w.name.toStdString();
            backend["type"] = w.type.toStdString();
            if (backend["type"].getString() == "Client")
                backend["url"] = w.urlOrPath.toStdString();
            else
                backend["path"] = w.urlOrPath.toStdString();
            backend["graph"] = w.graph.toStdString();
            backend["readonly"] = w.readOnly;
            config["import_servers"].push_back(std::move(backend));
        }

        // user can click finish without adding servers
        config.toYamlFile(this->configFilename);
        done(0);
    }

    void MultiDBConfigDialog::onTableBackendsCellChange(int row, int column)
    {
        if (column == 4) // ReadOnly column of the table
        {
           if(areAllImportserversReadonly()) {
             cNewReadOnly->setCheckState(Qt::Unchecked);
           } else {
            cNewReadOnly->setCheckState(Qt::Checked);
                
           }
            updateTableCheckboxes(Qt::Checked, row);

        }
        switch (column)
        {
        case 0: // backend name
            backends[row].name = tableBackends->item(row, column)->text();
            break;
        case 1: // backend type
            backends[row].type = tableBackends->item(row, column)->text();
            break;
        case 2: // URL/path
            backends[row].urlOrPath = tableBackends->item(row, column)->text();
            break;
        case 3: // graph
            backends[row].graph = tableBackends->item(row, column)->text();
            break;
        case 4: // readOnly
            backends[row].readOnly = (tableBackends->item(row, column)->checkState() == Qt::Checked);
            break;
        default:
            throw std::runtime_error("Invalid table column");
        }
    }
    void MultiDBConfigDialog::updateTableCheckboxes(Qt::CheckState state, int excludeRow)
    {
        for (int row = 0; row < tableBackends->rowCount(); ++row)
        {
            if (row != excludeRow)
            {
                QTableWidgetItem *item = tableBackends->item(row, 4); // 4 is the column for ReadOnly
                if (item)
                {
                    item->setCheckState(state);
                }
            }
        }
    }

    void MultiDBConfigDialog::onMainserverReadOnlyCheckboxStateChange()
    {
        if (cNewReadOnly->checkState() == Qt::Checked) {
            if(areAllImportserversReadonly()) {
                QMessageBox::warning(this, "Warning", "Please uncheck one of the importserver to make it writeable .", QMessageBox::Ok);
                cNewReadOnly->setCheckState(Qt::Unchecked);
            } else {
                // ok.
            }
        } else {
            updateTableCheckboxes(Qt::Checked, -1); // Check all checkboxes in the table
    
        }
        
    }
    
    bool MultiDBConfigDialog::areAllImportserversReadonly() {
        bool allChecked = true;
        for (int row = 0; row < tableBackends->rowCount(); ++row)
        {
            QTableWidgetItem *item = tableBackends->item(row, 4); // 4 is the ReadOnly column
            if (item && item->checkState() != Qt::Checked)
            {
                allChecked = false;
                break;
            }
        }
        return allChecked;
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
        delete vLayout;
        delete cbMainServerType;
        delete lbMainServerPathOrUrl;
        delete tfMainServerPath;
        delete tfMainServerGraph;

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
