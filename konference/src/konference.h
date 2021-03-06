
#ifndef _KONFERENCE_H_
#define _KONFERENCE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kapplication.h>
#include <kparts/mainwindow.h>

class KToggleAction;

/**
 * This is the application "Shell".  It has a menubar, toolbar, and
 * statusbar but relies on the "Part" to do all the real work.
 *
 * @short Application Shell
 * @author Malte B�hme <malte.boehme@rwth-aachen.de>
 * @version 0.1
 */
class Konference : public KParts::MainWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    Konference();

    /**
     * Default Destructor
     */
    virtual ~Konference();

    /**
     * Use this method to load whatever file/URL you have
     */
    void load(const KURL& url);

protected:
    /**
     * This method is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfig *);

    /**
     * This method is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(KConfig *);

private slots:
    void optionsShowToolbar();
    void optionsShowStatusbar();
    void optionsConfigureKeys();
    void optionsConfigureToolbars();

    void applyNewToolbarConfig();

private:
    void setupAccel();
    void setupActions();

private:
    KParts::ReadWritePart *m_part;

    KToggleAction *m_toolbarAction;
    KToggleAction *m_statusbarAction;
};

#endif // _KONFERENCE_H_
