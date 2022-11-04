#include "ImportDialog.hpp"
#include <mars/config_map_gui/DataWidget.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QDesktopServices>
#include <array>

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

    std::string ImportDialog::lastDomain = "software";
    std::string ImportDialog::lastFilter = "";

    ImportDialog::ImportDialog(XRockGUI *xrockGui, bool load) : xrockGui(xrockGui), load(load),
                                                                selectedDomain(""),
                                                                selectedModel(""),
                                                                selectedVersion("")
    {

        // get data from database
        QHBoxLayout *mainLayout = new QHBoxLayout();
        QVBoxLayout *vLayout = new QVBoxLayout();

        domainSelect = new QComboBox();
        vLayout->addWidget(domainSelect);

        indexMap["software"] = 0;

        for (auto it : indexMap)
        {
            domainSelect->addItem(it.first.c_str());
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
        if (load)
        {
            this->setWindowTitle("Load Model/Component");
            button->setText("load model");
        }
        else
        {
            this->setWindowTitle("Add Model/Component");
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
        if ((int)indexMap[lastDomain] == 0)
        {
            changeDomain(lastDomain.c_str());
        }
        else
        {
            domainSelect->setCurrentIndex(indexMap[lastDomain]);
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
        ConfigMap map = xrockGui->db->requestModel(selectedDomain, selectedModel, selectedVersion, true);
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
            if (load)
            {
                xrockGui->loadComponent(selectedDomain, selectedModel, selectedVersion);
            }
            else
            {
                xrockGui->addComponent(selectedDomain, selectedModel, selectedVersion);
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
