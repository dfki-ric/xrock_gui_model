#include "ImportDialog.hpp"
#include <config_map_gui/DataWidget.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QDesktopServices>
#include <array>
#include "utils/WaitCursorRAII.hpp"

using namespace configmaps;
namespace xrock_gui_model
{

    std::string getHtml(const std::string &markdown)
    {
        std::string cmd = "echo \"" + markdown + "\" | python -m markdown";
        std::array<char, 128> buffer;
        std::string result;
        std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe)
            throw std::runtime_error("popen() failed!");
        while (!feof(pipe.get()))
        {
            if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
                result += buffer.data();
        }
        return result;
    }

    std::string ImportDialog::lastDomain = "SOFTWARE";
    std::string ImportDialog::lastFilter = "";

    ImportDialog::ImportDialog(XRockGUI *xrockGui, Intention intent) : xrockGui(xrockGui), intent(intent),
                                                                selectedDomain(""),
                                                                selectedModel(""),
                                                                selectedVersion("")
    {

        // get data from database
        QHBoxLayout *mainLayout = new QHBoxLayout();
        QVBoxLayout *vLayout = new QVBoxLayout();

        domainSelect = new QComboBox();
        vLayout->addWidget(domainSelect);

        std::vector<std::string> domains = xrockGui->db->getDomains();
        int index = 0;
        for (const auto &d : domains)
        {
            if(intent == Intention::SELECT_HARDWARE)
                if(d != "ASSEMBLY")
                    continue;
            domainSelect->addItem(QString::fromStdString(d));
            indexMap[d] = index;
            index++;
        }

        connect(domainSelect, SIGNAL(currentIndexChanged(const QString &)),
                this, SLOT(changeDomain(const QString &)));

        QLabel *label = new QLabel("name/type filter:");
        vLayout->addWidget(label);
        filterPattern = new QLineEdit();
        filterPattern->setText(lastFilter.c_str());
        vLayout->addWidget(filterPattern);
        connect(filterPattern, SIGNAL(textChanged(const QString &)),
                this, SLOT(updateFilter(const QString &)));

        models = new QListWidget(this);
        models->setSelectionMode(QAbstractItemView::SingleSelection);
        vLayout->addWidget(models);
        connect(models, SIGNAL(clicked(const QModelIndex &)),
                this, SLOT(modelClicked(const QModelIndex &)));

        versionLabel = new QLabel("Version:");
        vLayout->addWidget(versionLabel);

        versionSelect = new QComboBox();
        versionSelect->setInsertPolicy(QComboBox::InsertAlphabetically);
        vLayout->addWidget(versionSelect);
        connect(versionSelect, SIGNAL(currentIndexChanged(const QString &)),
                this, SLOT(versionChanged(const QString &)));

        dw = new mars::config_map_gui::DataWidget(NULL, this, true, false);

        vLayout->addWidget(dw);
        std::vector<std::string> pattern;
        pattern.push_back("-"); // fake patter to activate check
        dw->setEditPattern(pattern);
        pattern.clear();
        pattern.push_back("components*");
        dw->setBlackFilterPattern(pattern);

        QPushButton *button = new QPushButton("add component");

        switch (intent)
        {
        case Intention::LOAD_CM:
            this->setWindowTitle("Load Model/Component");
            button->setText("load model");
            break;
        case Intention::ADD_CM:
            this->setWindowTitle("Add Model/Component");
            button->setText("add model");
            break;
        case Intention::ADD_TYPE:
            this->setWindowTitle("Add Type");
            button->setText("add type");
            break;

        case Intention::SELECT_HARDWARE:
            this->setWindowTitle("Add Hardware Model");
            button->setText("add hardware");
            changeDomain("ASSEMBLY");
            break;

        default:
            break;
        }

        vLayout->addWidget(button);
        connect(button, SIGNAL(clicked()), this, SLOT(addModel()));

        doc = new QWebView();
        doc->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
        connect(doc, SIGNAL(linkClicked(const QUrl &)), this, SLOT(urlClicked(const QUrl &)));
        mainLayout->addLayout(vLayout);
        mainLayout->addWidget(doc);
        setLayout(mainLayout);
        doc->setHtml("");
        doc->setStyleSheet("background-color:#eeeeee;");
        if(intent != Intention::SELECT_HARDWARE) {      
            if ((int)indexMap[lastDomain] == 0)
            {
                changeDomain(lastDomain.c_str());
            }
            else
            {
                domainSelect->setCurrentIndex(indexMap[lastDomain]);
            }
        }

        if (!lastFilter.empty())
        {
            updateFilter(lastFilter.c_str());
        }
    }

    ImportDialog::~ImportDialog()
    {
    }

    void ImportDialog::urlClicked(const QUrl &link)
    {
        QDesktopServices::openUrl(link);
    }

    void ImportDialog::modelClicked(const QModelIndex &index)
    {
        ignoreUpdate = true;
        std::string firstVersion;
        QVariant v = models->model()->data(index, 0);
        if (v.isValid())
        {
            WaitCursorRAII _;
            selectedModel = v.toString().toStdString();
            selectedVersion = std::string("");
            dw->clearGUI();
            versionSelect->clear();
            std::vector<std::string> versionList = xrockGui->db->requestVersions(selectedDomain, selectedModel);
            for (std::vector<std::string>::iterator it = versionList.begin(); it != versionList.end(); ++it)
            {
                if (firstVersion.empty())
                {
                    firstVersion = (*it);
                }
                versionSelect->addItem((*it).c_str());
            }
        }
        ignoreUpdate = false;
        if (!firstVersion.empty())
        {
            versionChanged(firstVersion.c_str());
        }
    }

    void ImportDialog::versionChanged(const QString &versionName)
    {
        if (ignoreUpdate)
            return;
        selectedVersion = versionName.toStdString();
        dw->clearGUI();
        ConfigMap map;
        {
                WaitCursorRAII _;
                map = xrockGui->db->requestModel(selectedDomain, selectedModel, selectedVersion, true);
        }
        if (intent == Intention::ADD_TYPE || intent == Intention::SELECT_HARDWARE)
        {
            model = map;
        }
            doc->setHtml("");
            if (map["versions"][0].hasKey("data"))
            {
                {
                    ConfigMap dataMap;
                    if (map["versions"][0]["data"].isMap())
                        dataMap = map["versions"][0]["data"];
                    else
                        dataMap = ConfigMap::fromYamlString(map["versions"][0]["data"]);
                    if (dataMap.hasKey("description"))
                    {
                        if (dataMap["description"].hasKey("markdown"))
                        {
                            std::string md = dataMap["description"]["markdown"];
                            doc->setHtml(getHtml(md).c_str());
                        }
                    }
                }
            }
            dw->setConfigMap("", map);
        }

    void ImportDialog::addModel()
    {
        if (selectedDomain != std::string("") &&
            selectedModel != std::string("") &&
            selectedVersion != std::string(""))
        {

            switch (intent)
            {
            case Intention::LOAD_CM:
            {
                xrockGui->loadComponentModel(selectedDomain, selectedModel, selectedVersion);
                break;
            }
            case Intention::ADD_CM:
            {
                xrockGui->addComponent(selectedDomain, selectedModel, selectedVersion);
                break;
            }
            default:
                break;
            }
            done(0);
        }
    }

    void ImportDialog::updateFilter(const QString &filter)
    {
        QRegExp exp(filter, Qt::CaseInsensitive);

        models->clear();
        for (auto it : modelList)
        {
            if (exp.indexIn(it.first.c_str()) != -1 ||
                exp.indexIn(it.second.c_str()) != -1)
            {
                models->addItem(it.first.c_str());
            }
        }
        lastFilter = filter.toStdString();
    }

    void ImportDialog::changeDomain(const QString &domain)
    {
        WaitCursorRAII _;
        models->clear();
        versionSelect->clear();
        dw->clearGUI();
        selectedDomain = domain.toStdString();
        selectedModel = std::string("");
        selectedVersion = std::string("");
        modelList = xrockGui->db->requestModelListByDomain(selectedDomain);
        std::sort(modelList.begin(), modelList.end());
        auto last = std::unique(modelList.begin(), modelList.end());
        modelList.erase(last, modelList.end());

        for (auto it : modelList)
        {
            models->addItem(it.first.c_str());
        }
        models->sortItems();
        lastDomain = selectedDomain;
    }

} // end of namespace xrock_gui_model
