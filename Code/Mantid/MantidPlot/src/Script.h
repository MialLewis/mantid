/***************************************************************************
    File                 : Script.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Scripting abstraction layer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef SCRIPT_H
#define SCRIPT_H

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QObject>
#include <QStringList>
#include <QEvent>

#include "ScriptingEnv.h"

//-------------------------------------------
// Forward declarations
//-------------------------------------------
class ApplicationWindow;

/**
 * Script objects represent a chunk of code, possibly together with local
 * variables. The code may be changed and executed multiple times during the
 * lifetime of an object.
 */
class Script : public QObject
{
  Q_OBJECT

  public:
  /// Constructor
  Script(ScriptingEnv *env, const QString &code, QObject *context=NULL, 
	 const QString &name="<input>", bool interactive = true, bool reportProgress = false);
  /// Destructor
  virtual ~Script();
  /// Return the code that will be executed when calling exec() or eval()
  const QString code() const { return Code; }
  /// Return the context in which the code is to be executed.
  const QObject* context() const { return Context; }
  /// Like QObject::name, but with unicode support.
  const QString name() const { return Name; }
  /// Return whether errors / exceptions are to be emitted or silently ignored
  bool emitErrors() const { return EmitErrors; }
  /// Append to the code that will be executed when calling exec() or eval()
  virtual void addCode(const QString &code);
  /// Set the code that will be executed when calling exec() or eval()
  virtual void setCode(const QString &code);
  /// Set the context in which the code is to be executed.
  virtual void setContext(QObject *context) { Context = context; compiled = notCompiled; }
  /// Like QObject::setName, but with unicode support.
  void setName(const QString &name) { Name = name; compiled = notCompiled; }
  /// Set whether errors / exceptions are to be emitted or silently ignored
  void setEmitErrors(bool yes) { EmitErrors = yes; }
  /// Whether we should be reporting progress  
  bool reportProgress() const { return (m_interactive && m_report_progress); }
  //!Set whether we should be reporting progress
  void reportProgress(bool on) 
  { 
    if( Env->supportsProgressReporting() && m_interactive ) m_report_progress = on; 
    else m_report_progress = false;
  }
  // Set the line offset of the current code
  void setLineOffset(int offset) { m_line_offset = offset; } 
  // The current line offset
  int getLineOffset() const { return m_line_offset; } 
  // Is anything running
  bool scriptIsRunning() { return Env->isRunning(); }
public slots:
  /// Compile the Code. Return true if the implementation doesn't support compilation.
  virtual bool compile(bool for_eval=true);
  /// Evaluate the Code, returning QVariant() on an error / exception.
  virtual QVariant eval();
  /// Execute the Code, returning false on an error / exception.
  virtual bool exec();
  // local variables
  virtual bool setQObject(const QObject*, const char*) { return false; }
  virtual bool setInt(int, const char*) { return false; }
  virtual bool setDouble(double, const char*) { return false; }
  
signals:
  /// This is emitted whenever the code to be executed by exec() and eval() is changed.
  void codeChanged();
  /// signal an error condition / exception
  void error(const QString & message, const QString & scriptName, int lineNumber);
  /// output generated by the code
  void print(const QString & output);
  /// Line number changed
  void currentLineChanged(int lineno, bool error);
  // Signal that new keywords are available
  void keywordsChanged(const QStringList & keywords);

protected:
  ScriptingEnv *Env;
  QString Code, Name;
  QObject *Context;
  /// Should this be an interactive environment?
  bool m_interactive;
  /// Is progress reporting on?
  bool m_report_progress;
  enum compileStatus { notCompiled, isCompiled, compileErr } compiled;
  bool EmitErrors;

  void emit_error(const QString & message, int lineNumber)
  { if(EmitErrors) emit error(message, Name, lineNumber); }
  
private:
  /// Normalise line endings for the given code. The Python C/API does not seem to like CRLF endings so normalise to just LF
  QString normaliseLineEndings(QString text) const;

private:
  int m_line_offset;
};




#endif
