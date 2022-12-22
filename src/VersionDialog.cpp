#include "VersionDialog.hpp"
#include "XRockGUI.hpp"
#include <mars/config_map_gui/DataWidget.h>
#include <mars/utils/misc.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QSplitter>

using namespace configmaps;

namespace xrock_gui_model
{

    VersionDialog::VersionDialog(XRockGUI *xrockGui) : xrockGui(xrockGui)
    {
        // get data from database
        QSplitter *split = new QSplitter();
        QVBoxLayout *vLayout = new QVBoxLayout();

        vLayout->addWidget(split);
        versions = new QListWidget(this);
        split->addWidget(versions);
        connect(versions, SIGNAL(clicked(const QModelIndex &)),
                this, SLOT(versionClicked(const QModelIndex &)));
        connect(versions, SIGNAL(activated(const QModelIndex &)),
                this, SLOT(versionActivated(const QModelIndex &)));

        dw = new mars::config_map_gui::DataWidget(NULL, this, true, false);
        split->addWidget(dw);
        std::vector<std::string> editPattern;
        editPattern.push_back("-"); // fake patter to activate check
        dw->setEditPattern(editPattern);

        QPushButton *button = new QPushButton("select version");
        vLayout->addWidget(button);
        connect(button, SIGNAL(clicked()), this, SLOT(selectVersion()));
        setLayout(vLayout);
    }

    VersionDialog::~VersionDialog()
    {
    }

    void VersionDialog::requestComponent(const std::string &domain,
                                         const std::string &name)
    {
        selectedDomain = domain;
        selectedModel = name;
        std::vector<std::string> versionList = xrockGui->db->requestVersions(domain, name);
        versions->clear();
        for (std::vector<std::string>::iterator it = versionList.begin(); it != versionList.end(); ++it)
        {
            versions->addItem((*it).c_str());
        }
    }

    void VersionDialog::versionClicked(const QModelIndex &index)
    {
        QVariant v = versions->model()->data(index, 0);
        if (v.isValid())
        {
            selectedVersion = v.toString().toStdString();
            dw->clearGUI();
            ConfigMap map = xrockGui->db->requestModel(selectedDomain, selectedModel, selectedVersion);
            dw->setConfigMap("", map);
        }
    }

    void VersionDialog::versionActivated(const QModelIndex &index)
    {
        QVariant v = versions->model()->data(index, 0);
        if (v.isValid())
        {
            selectedVersion = v.toString().toStdString();
            selectVersion();
        }
    }

    void VersionDialog::selectVersion()
    {
        if (selectedVersion != std::string(""))
        {
            xrockGui->selectVersion(selectedVersion);
        }
        done(0);
    }

} // end of namespace xrock_gui_model
