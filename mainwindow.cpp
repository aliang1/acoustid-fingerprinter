#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTreeView>
#include <QPushButton>
#include <QDebug>
#include <QSettings>
#include "progressdialog.h"
#include "checkabledirmodel.h"
#include "fingerprinter.h"
#include "mainwindow.h"
#include "constants.h"

MainWindow::MainWindow()
{
	setupUi();
}

void MainWindow::setupUi()
{

	QTreeView *treeView = new QTreeView();

	m_directoryModel = new CheckableDirModel();
	m_directoryModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
#ifdef Q_WS_MAC
	m_directoryModel->setRootPath("/Volumes");
#else
	m_directoryModel->setRootPath("");
#endif

	treeView->setModel(m_directoryModel);
	treeView->setHeaderHidden(true);
	treeView->hideColumn(1);
	treeView->hideColumn(2);
	treeView->hideColumn(3);

//	QString homePath = QDir::homePath();
	QString homePath = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
	const QModelIndex homePathIndex = m_directoryModel->index(homePath);
	treeView->expand(homePathIndex);
	treeView->selectionModel()->setCurrentIndex(homePathIndex, QItemSelectionModel::ClearAndSelect);
	treeView->scrollTo(homePathIndex);

	QLabel *treeViewLabel = new QLabel(tr("&Select which folders to fingerprint:"));
	treeViewLabel->setBuddy(treeView);

	m_apiKeyEdit = new QLineEdit();
	QPushButton *apiKeyButton = new QPushButton(tr("&Get API key..."));
	connect(apiKeyButton, SIGNAL(clicked()), SLOT(openAcoustidWebsite()));

	QSettings settings;
	m_apiKeyEdit->setText(settings.value("apikey").toString());

	QHBoxLayout *apiKeyLayout = new QHBoxLayout();
	apiKeyLayout->addWidget(m_apiKeyEdit);
	apiKeyLayout->addWidget(apiKeyButton);

	QLabel *apiKeyLabel = new QLabel(tr("Your Acoustid &API key:"));
	apiKeyLabel->setBuddy(m_apiKeyEdit);

	QPushButton *fingerprintButton = new QPushButton(tr("&Fingerprint..."));
	connect(fingerprintButton, SIGNAL(clicked()), SLOT(fingerprint()));

	QPushButton *closeButton = new QPushButton(tr("&Close"));
	connect(closeButton, SIGNAL(clicked()), SLOT(close()));

	QDialogButtonBox *buttonBox = new QDialogButtonBox();
	buttonBox->addButton(fingerprintButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(closeButton, QDialogButtonBox::RejectRole);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(apiKeyLabel);
	mainLayout->addLayout(apiKeyLayout);
	mainLayout->addWidget(treeViewLabel);
	mainLayout->addWidget(treeView);
	mainLayout->addWidget(buttonBox);

	QWidget *centralWidget = new QWidget();
	centralWidget->setLayout(mainLayout);
	setCentralWidget(centralWidget);
	setWindowTitle(tr("Acoustid Fingerprinter"));
	QIcon icon;
	icon.addFile(":/images/acoustid-fp-16.png", QSize(16, 16));
	icon.addFile(":/images/acoustid-fp-24.png", QSize(24, 24));
	icon.addFile(":/images/acoustid-fp-32.png", QSize(32, 32));
	icon.addFile(":/images/acoustid-fp-48.png", QSize(48, 48));
	setWindowIcon(icon);
	resize(QSize(400, 500));
}

void MainWindow::openAcoustidWebsite()
{
	QDesktopServices::openUrl(QUrl::fromPercentEncoding(API_KEY_URL));
}

void MainWindow::fingerprint()
{
	QString apiKey;
	QList<QString> directories;
	if (!validateFields(apiKey, directories)) {
		return;
	}
	QSettings settings;
	settings.setValue("apikey", apiKey);
	Fingerprinter *fingerprinter = new Fingerprinter(apiKey, directories);
    ProgressDialog *progressDialog = new ProgressDialog(this, fingerprinter);
	fingerprinter->start();
    progressDialog->setModal(true);
    progressDialog->show();
}

bool MainWindow::validateFields(QString &apiKey, QList<QString> &directories)
{
	apiKey = m_apiKeyEdit->text();
	if (apiKey.isEmpty()) {
		QMessageBox::warning(this, tr("Error"),
			tr("Please enter your Acoustid API key. You can get an API key "
			"from the <a href=\"%1\">Acoustid website</a> after signing in "
			"with your MusicBrainz account or any OpenID (Google, Yahoo, "
			"etc.)").arg(API_KEY_URL));
		return false;
	}
	directories = m_directoryModel->selectedDirectories();
	if (directories.isEmpty()) {
		QMessageBox::warning(this, tr("Error"),
			tr("Please select one or more folders with audio files to fingerprint."));
		return false;
	}
	return true;
}

