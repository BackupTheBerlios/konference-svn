
#include "konference.h"

#include <kkeydialog.h>
#include <kfiledialog.h>
#include <kconfig.h>
#include <kurl.h>

#include <kedittoolbar.h>

#include <kaction.h>
#include <kstdaction.h>

#include <klibloader.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <klocale.h>

Konference::Konference() : KParts::MainWindow( 0L, "Konference" )
{
	// set the shell's ui resource file
	setXMLFile("konference_shell.rc");

	// then, setup our actions
	setupActions();

	// and a status bar
	statusBar()->show();

	// this routine will find and load our Part.  it finds the Part by
	// name which is a bad idea usually.. but it's alright in this
	// case since our Part is made for this Shell
	KLibFactory *factory = KLibLoader::self()->factory("libkonferencepart");
	if (factory)
	{
		// now that the Part is loaded, we cast it to a Part to get
		// our hands on it
		m_part = static_cast<KParts::ReadWritePart *>(factory->create(this,
		         "konference_part", "KParts::ReadWritePart" ));

		if (m_part)
		{
			// tell the KParts::MainWindow that this is indeed the main widget
			setCentralWidget(m_part->widget());

			// and integrate the part's GUI with the shell's
			createGUI(m_part);
		}
	}
	else
	{
		// if we couldn't find our Part, we exit since the Shell by
		// itself can't do anything useful
		KMessageBox::error(this, i18n("Could not find our part."));
		kapp->quit();
		// we return here, cause kapp->quit() only means "exit the
		// next time we enter the event loop...
		return;
	}

	// apply the saved mainwindow settings, if any, and ask the mainwindow
	// to automatically save settings if changed: window size, toolbar
	// position, icon size, etc.
	setAutoSaveSettings();
}

Konference::~Konference()
{}

void Konference::load(const KURL& url)
{
	m_part->openURL( url );
}

void Konference::setupActions()
{
	//KStdAction::openNew(this, SLOT(fileNew()), actionCollection());
	//KStdAction::open(this, SLOT(fileOpen()), actionCollection());

	KStdAction::quit(kapp, SLOT(quit()), actionCollection());

	m_toolbarAction = KStdAction::showToolbar(this, SLOT(optionsShowToolbar()), actionCollection());
	m_statusbarAction = KStdAction::showStatusbar(this, SLOT(optionsShowStatusbar()), actionCollection());

	KStdAction::keyBindings(this, SLOT(optionsConfigureKeys()), actionCollection());
	KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());
}

void Konference::saveProperties(KConfig* /*config*/)
{
	// the 'config' object points to the session managed
	// config file.  anything you write here will be available
	// later when this app is restored
}

void Konference::readProperties(KConfig* /*config*/)
{
	// the 'config' object points to the session managed
	// config file.  this function is automatically called whenever
	// the app is being restored.  read in here whatever you wrote
	// in 'saveProperties'
}

void Konference::optionsShowToolbar()
{
	// this is all very cut and paste code for showing/hiding the
	// toolbar
	if (m_toolbarAction->isChecked())
		toolBar()->show();
	else
		toolBar()->hide();
}

void Konference::optionsShowStatusbar()
{
	// this is all very cut and paste code for showing/hiding the
	// statusbar
	if (m_statusbarAction->isChecked())
		statusBar()->show();
	else
		statusBar()->hide();
}

void Konference::optionsConfigureKeys()
{
	KKeyDialog::configure(actionCollection());
}

void Konference::optionsConfigureToolbars()
{
	saveMainWindowSettings(KGlobal::config() );

	// use the standard toolbar editor
	KEditToolbar dlg(factory());
	connect(&dlg, SIGNAL(newToolbarConfig()),
	        this, SLOT(applyNewToolbarConfig()));
	dlg.exec();
}

void Konference::applyNewToolbarConfig()
{
	applyMainWindowSettings(KGlobal::config());
}


#include "konference.moc"
