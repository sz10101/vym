#include "scripting.h"

#include "branchitem.h"
#include "imageitem.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodel.h"
#include "vymtext.h"
#include "xlink.h"

extern Main *mainWindow;

///////////////////////////////////////////////////////////////////////////
void logError(QScriptContext *context, QScriptContext::Error error, const QString &text)
{
    if (context)
        context->throwError( error, text);
    else
        qDebug()<<"VymWrapper: "<<text;
}

///////////////////////////////////////////////////////////////////////////
VymModelWrapper::VymModelWrapper(VymModel *m)
{
    model = m;
}

BranchItem*  VymModelWrapper::getSelectedBranch()
{
    BranchItem *selbi = model->getSelectedBranch();
    if (!selbi) logError( context(),  QScriptContext::ReferenceError, "No branch selected" );
    return selbi;
}

QVariant VymModelWrapper::getParameter( bool &ok, const QString &key, const QStringList &parameters )
{
    // loop through parameters and try to find the one named "key"
    foreach ( QString par, parameters)
    {
        if ( par.startsWith( key ) )
        {
            qDebug()<<"getParam: "<<key<<"  has: "<< par;
            ok = true;
            return QVariant( par );
        }
    }

    // Nothing found
    ok = false;
    return QVariant::Invalid;
}

void VymModelWrapper::addBranch()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        if (! model->addNewBranch() )
            logError( context(), QScriptContext::UnknownError, "Couldn't add branch to map");
    } 
}

void VymModelWrapper::addBranchBefore()
{
    if (! model->addNewBranchBefore() )
        logError( context(), QScriptContext::UnknownError, "Couldn't add branch before selection to map");
}

void VymModelWrapper::addMapCenter( qreal x, qreal y)
{
    if (! model->addMapCenter( QPointF (x, y) ) )
        logError( context(), QScriptContext::UnknownError, "Couldn't add mapcenter");
}

void VymModelWrapper::addMapInsert( QString fileName, int pos, int contentFilter)
{
    if (QDir::isRelativePath( fileName )) 
        fileName = QDir::currentPath() + "/" + fileName;

    model->saveStateBeforeLoad (ImportAdd, fileName);

    if (File::Aborted == model->loadMap (fileName, ImportAdd, VymMap, contentFilter, pos) )
        logError( context(), QScriptContext::UnknownError, QString( "Couldn't load %1").arg(fileName) );
}

void VymModelWrapper::addMapInsert( const QString &fileName, int pos)
{
    addMapInsert( fileName, pos, 0x0000);
}

void VymModelWrapper::addMapInsert( const QString &fileName)
{
    addMapInsert( fileName, -1, 0x0000);
}

void VymModelWrapper::addMapReplace( QString fileName )
{
    if (QDir::isRelativePath( fileName )) 
        fileName = QDir::currentPath() + "/" + fileName;

    model->saveStateBeforeLoad (ImportReplace, fileName);

    if (File::Aborted == model->loadMap (fileName, ImportReplace, VymMap) )
        logError( context(), QScriptContext::UnknownError, QString( "Couldn't load %1").arg(fileName) );
}

void VymModelWrapper::addSlide()
{
    model->addSlide();
}

void VymModelWrapper::addXLink( const QString &begin, const QString &end, int width, const QString &color, const QString &penstyle)
{
    BranchItem *bbegin = (BranchItem*)(model->findBySelectString( begin ) );
    BranchItem *bend = (BranchItem*)(model->findBySelectString( end ) );
    if (bbegin && bend)
    {
        if (bbegin->isBranchLikeType() && bend->isBranchLikeType())
        {
            Link *li = new Link ( model );
            li->setBeginBranch ( (BranchItem*)bbegin );
            li->setEndBranch ( (BranchItem*)bend);

            model->createLink (li);
            QPen pen = li->getPen();
            if (width > 0 ) pen.setWidth( width );
            QColor col (color);
            if (col.isValid()) pen.setColor ( col );

            bool ok;
            Qt::PenStyle st1 = penStyle ( penstyle, ok);
            if (ok) 
            {
                pen.setStyle ( st1 );
                li->setPen( pen );	
            } else	
                logError( context(), QScriptContext::UnknownError, QString("Couldn't set penstyle %1").arg(penstyle));
        }
        else
            logError( context(), QScriptContext::UnknownError, "Begin or end of xLink are not branch or mapcenter");
        
    } else
        logError( context(), QScriptContext::UnknownError, "Begin or end of xLink not found");
}

int VymModelWrapper::branchCount()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        return selbi->branchCount();
    else
        return -1;
}

int VymModelWrapper::centerCount()
{
    return model->centerCount();
}

void VymModelWrapper::centerOnID( const QString &id)
{
    if ( !model->centerOnID( id ) ) 
        logError( context(), QScriptContext::UnknownError, QString("Could not center on ID %1").arg(id) );
}

void VymModelWrapper::clearFlags()
{
    return model->clearFlags();
}

void VymModelWrapper::colorBranch( const QString &color)
{
    QColor col(color);
    if ( !col.isValid() )
        logError( context(), QScriptContext::SyntaxError, QString( "Couldn't parse color %1").arg(color) );
    else
        model->colorBranch( col );
}

void VymModelWrapper::colorSubtree( const QString &color)
{
    QColor col(color);
    if ( !col.isValid() )
        logError( context(), QScriptContext::SyntaxError, QString( "Couldn't parse color %1").arg(color) );
    else
        model->colorSubtree( col );
}

void VymModelWrapper::copy()
{
    model->copy();
}

void VymModelWrapper::cut()
{
    model->cut();
}

void VymModelWrapper::cycleTask()
{
    if ( !model->cycleTaskStatus() )
        logError( context(), QScriptContext::SyntaxError, "Couldn't cycle task status");
}

bool VymModelWrapper::exportMap( const QString &format, const QStringList &parameters)
{
    QString filename;
    bool ok;

    filename = getParameter( ok, "filename", parameters).toString();

    if ( !ok && format != "Last" )
    {
        logError( context(), QScriptContext::SyntaxError, QString("Filename missing in export to %1").arg(format) );
        return false;
    }

    if (format == "AO") 
    {
        model->exportAO (filename, false);
    } else if ( format == "ASCII" ) 
    {
        bool listTasks = getParameter( ok, "listTasks", parameters).toBool();
        model->exportASCII (listTasks, filename, false);
    } else if ( format == "CSV" )
    {
        model->exportCSV (filename, false);
    } else if ( format == "HTML" )
    {
        QString path = getParameter( ok, "path", parameters).toString();
        if ( !ok )
        {
            logError( context(), QScriptContext::SyntaxError, QString("Path missing in export to %1").arg(format) );
            return false;
        }   
        model->exportHTML (path, filename, false);
    } else if ( format == "Image" )
    {
        QString format;
        format = getParameter( ok, "format", parameters).toString();
        if (!ok)
            format = "PNG";
        else
        {
            QStringList formats;
            formats << "PNG"; 
            formats << "GIF"; 
            formats << "JPG"; 
            formats << "JPEG", 
            formats << "PNG", 
            formats << "PBM", 
            formats << "PGM", 
            formats << "PPM", 
            formats << "TIFF", 
            formats << "XBM", 
            formats << "XPM";
            if ( formats.indexOf( format ) < 0 )
            {
                logError( context(), QScriptContext::SyntaxError, QString("%1 not one of the known export formats: ").arg(format).arg(formats.join(",") ) );
                return false;
            }
        }
        model->exportImage ( filename, false, format);
    } else if ( format == "Impress" )
    {
        QString templ = getParameter( ok, "template", parameters).toString();
        if ( !ok )
        {
            logError( context(), QScriptContext::SyntaxError, "Template missing in exportImpress");
            return false;
        }
        model->exportImpress (filename, templ);
    } else if ( format == "Last" )
    {
        model->exportLast();
    } else if ( format == "LaTeX" )
    {
        model->exportLaTeX (filename, false);
    } else if ( format == "OrgMode" )
    {
        model->exportOrgMode ( filename,false);
    } else if ( format == "PDF" )
    {
        model->exportPDF( filename, false);
    } else if ( format == "SVG" )
    {
        model->exportPDF( filename, false);
    } else if ( format == "XML" )
    {
        QString path = getParameter( ok, "path", parameters).toString();
        if ( !ok )
        {
            logError( context(), QScriptContext::SyntaxError, QString("Path missing in export to %1").arg(format) );
            return false;
        }   
        model->exportXML (path, filename, false);
    } else
    {
        logError( context(), QScriptContext::SyntaxError, QString("Unknown export format: %1").arg(format) );
        return false;
    }
    return true;
}

QString VymModelWrapper::getDestPath()
{
    return model->getDestPath();
}

QString VymModelWrapper::getFileDir()
{
    return model->getFileDir();
}

QString VymModelWrapper::getFileName()
{
    return model->getFileName();
}

QString VymModelWrapper::getFrameType()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        BranchObj *bo = (BranchObj*)(selbi->getLMO());
        if (!bo)
            logError( context(), QScriptContext::UnknownError, QString("No BranchObj available") );
        else
            return bo->getFrame()->getFrameTypeName();
    } 
    return QString();
}

QString VymModelWrapper::getHeadingPlainText()
{
    return model->getHeading().getTextASCII();
}

QString VymModelWrapper::getHeadingXML()
{
    return model->getHeading().saveToDir();
}

QString VymModelWrapper::getMapAuthor()
{
    return model->getAuthor();
}

QString VymModelWrapper::getMapComment()
{
    return model->getComment();
}

QString VymModelWrapper::getMapTitle()
{
    return model->getTitle();
}

QString VymModelWrapper::getNotePlainText()
{
    return model->getHeading().getTextASCII(); 
}

QString VymModelWrapper::getNoteXML()
{
    return model->getHeading().saveToDir(); 
}

QString VymModelWrapper::getSelectString()
{
    return model->getSelectString();
}

void VymModelWrapper::moveDown()
{
    model->moveDown();
}

void VymModelWrapper::moveUp()
{
    model->moveUp();
}

void VymModelWrapper::nop() {}

void VymModelWrapper::paste()
{
    model->paste();
}

void VymModelWrapper::redo()
{
    model->redo();
}

void VymModelWrapper::remove()
{
    model->deleteSelection();
}

void VymModelWrapper::removeChildren()
{
    model->deleteChildren();
}

void VymModelWrapper::removeKeepChildren()
{
    model->deleteKeepChildren();
}

void VymModelWrapper::removeSlide(int n)
{
    if ( n < 0 || n >= model->slideCount() - 1)
        logError( context(), QScriptContext::RangeError, QString("Slide '%1' not available.").arg(n) );
}

void VymModelWrapper::scroll()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        if (! model->scrollBranch(selbi) )
            logError( context(), QScriptContext::UnknownError, "Couldn't scroll branch");
    } 
}

bool VymModelWrapper::select(const QString &s)
{
    bool r = model->select( s );
    if (!r) logError( context(), QScriptContext::UnknownError, QString("Couldn't select %1").arg(s));
    return r;
}

bool VymModelWrapper::selectID(const QString &s)
{
    bool r = model->selectID( s );
    if (!r) logError( context(), QScriptContext::UnknownError, QString("Couldn't select ID %1").arg(s));
    return r;
}

bool VymModelWrapper::selectFirstBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        r = model->selectFirstBranch();
        if (!r) logError( context(), QScriptContext::UnknownError, "Couldn't select first branch");
    }
    return r;
}

bool VymModelWrapper::selectLastBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        r = model->selectLastBranch();
        if (!r) logError( context(), QScriptContext::UnknownError, "Couldn't select last branch");
    }
    return r;
}

bool VymModelWrapper::selectLastImage()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        ImageItem* ii = selbi->getLastImage();
        if (!ii) 
            logError( context(), QScriptContext::UnknownError, "Couldn't get last image");
        else
        {
            r = model->select( ii );
            if (!r) 
                logError( context(), QScriptContext::UnknownError, "Couldn't select last image");
        }
    }
    return r;
}

bool VymModelWrapper::selectParent()
{
    bool r = model->selectParent();
    if (!r) 
        logError( context(), QScriptContext::UnknownError, "Couldn't select parent item");
    return r;
}

bool VymModelWrapper::selectLatestAdded()
{
    bool r =  model->selectLatestAdded();
    if (!r) 
        logError( context(), QScriptContext::UnknownError, "Couldn't select latest added item");
    return r;
}

void VymModelWrapper::setFlag(const QString &s)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) selbi->activateStandardFlag( s );
}

void VymModelWrapper::setHeadingPlainText(const QString &s)
{
    model->setHeading( s );
}

void VymModelWrapper::setMapAuthor(const QString &s)
{
    model->setAuthor( s );
}

void VymModelWrapper::setMapComment(const QString &s)
{
    model->setComment( s );
}

void VymModelWrapper::setMapRotation( float a)
{
    model->setMapRotationAngle( a );
}

void VymModelWrapper::setMapTitle(const QString &s)
{
    model->setTitle( s );
}

void VymModelWrapper::setMapZoom( float z)
{
    model->setMapZoomFactor( z );
}

void VymModelWrapper::setNotePlainText(const QString &s)
{
    VymNote vn;
    vn.setPlainText(s);
    model->setNote (vn);
}

void VymModelWrapper::setURL(const QString &s)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) selbi->setURL( s );
}

void VymModelWrapper::setVymLink(const QString &s)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) selbi->setVymLink( s );
}

void VymModelWrapper::sleep( int n)
{
    sleep( n );
}

void VymModelWrapper::sortChildren(bool b)
{
    model->sortChildren( b );
}

void VymModelWrapper::sortChildren()
{
    sortChildren( false );
}

void VymModelWrapper::toggleFlag(const QString &s)
{
    model->toggleStandardFlag( s );
}

void VymModelWrapper::toggleFrameIncludeChildren()
{
    model->toggleFrameIncludeChildren();
}

void VymModelWrapper::toggleScroll()
{
    model->toggleScroll();
}

void VymModelWrapper::toggleTarget()
{
    model->toggleTarget();
}

void VymModelWrapper::toggleTask()
{
    model->toggleTask();
}

void VymModelWrapper::undo()
{
    model->undo();
}

bool VymModelWrapper::unscroll()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        if ( model->unscrollBranch(selbi) )
            return true;
        else
            logError( context(), QScriptContext::UnknownError, "Couldn't unscroll branch");
    } 
    return false;
}

void VymModelWrapper::unscrollChildren()
{
    model->unscrollChildren();
}

void VymModelWrapper::unselectAll()
{
    model->unselectAll();
}

void VymModelWrapper::unsetFlag(const QString &s)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) selbi->deactivateStandardFlag( s );
}

///////////////////////////////////////////////////////////////////////////
VymWrapper::VymWrapper()
{
}

void VymWrapper::toggleTreeEditor()
{
    mainWindow->windowToggleTreeEditor();
}

QObject* VymWrapper::getCurrentMap()    // FIXME-1 No syntax highlighting
{
    return mainWindow->getCurrentModelWrapper();
}

void VymWrapper::selectMap(uint n)      // FIXME-1 No syntax highlighting
{
    if ( !mainWindow->gotoWindow( n ))
    {
        logError( context(), QScriptContext::RangeError, QString("Map '%1' not available.").arg(n) );
    }
}

