/*
# PostgreSQL Database Modeler (pgModeler)
#
# Copyright 2006-2021 - Raphael Araújo e Silva <raphael@pgmodeler.io>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# The complete text of GPLv3 is at LICENSE file on source code root directory.
# Also, you can get the complete GNU General Public License at <http://www.gnu.org/licenses/>
*/

#include "bugreportform.h"
#include "exception.h"
#include "globalattributes.h"
#include "messagebox.h"
#include "guiutilsns.h"
#include "utilsns.h"

BugReportForm::BugReportForm(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	setupUi(this);
	setWindowFlags(Qt::Dialog |
								 Qt::WindowTitleHint |
								 Qt::WindowMinMaxButtonsHint |
								 Qt::WindowCloseButtonHint);

	output_sel = new FileSelectorWidget(this);
	output_sel->setWindowTitle(tr("Select report output folder"));
	output_sel->setFileMode(QFileDialog::Directory);
	output_sel->setAllowFilenameInput(true);
	output_sel->setSelectedFile(GlobalAttributes::getTemporaryDir());

	output_lt->addWidget(output_sel);
	GuiUtilsNs::configureWidgetFont(hint_lbl, GuiUtilsNs::MediumFontFactor);

	connect(cancel_btn, SIGNAL(clicked()), this, SLOT(close()));
	connect(create_btn, SIGNAL(clicked()), this, SLOT(generateReport()));
	connect(attach_mod_chk, SIGNAL(toggled(bool)), attach_tb, SLOT(setEnabled(bool)));
	connect(attach_tb, SIGNAL(clicked()), this, SLOT(attachModel()));
	connect(details_txt, SIGNAL(textChanged()), this, SLOT(enableGeneration()));
	connect(output_sel, SIGNAL(s_selectorChanged(bool)), this, SLOT(enableGeneration()));

	//Installs a syntax highlighter on model_txt widget
	hl_model_txt=new SyntaxHighlighter(model_txt);
	hl_model_txt->loadConfiguration(GlobalAttributes::getXMLHighlightConfPath());

	try
	{
		attachModel(GlobalAttributes::getTemporaryFilePath(GlobalAttributes::LastModelFile));
	}
	catch(Exception &){}
}

void BugReportForm::attachModel(const QString &filename)
{
	model_txt->setPlainText(UtilsNs::loadFile(filename));
}

QByteArray BugReportForm::generateReportBuffer()
{
	QByteArray buf;

	buf.append(details_txt->toPlainText().toUtf8());
	buf.append(CharDelimiter);

	if(attach_mod_chk->isChecked())
		buf.append(model_txt->toPlainText().toUtf8());

	buf.append(CharDelimiter);
	return buf;
}

void BugReportForm::generateReport()
{ 
	generateReport(generateReportBuffer());
	this->accept();
}

void BugReportForm::enableGeneration()
{
	create_btn->setEnabled(!output_sel->getSelectedFile().isEmpty() &&
												 !output_sel->hasWarning() &&
												 !details_txt->toPlainText().isEmpty());
}

void BugReportForm::generateReport(const QByteArray &buf)
{
	Messagebox msgbox;
	QFile output;
	QString filename=QFileInfo(QString(output_sel->getSelectedFile() +
																		 GlobalAttributes::DirSeparator +
																		 GlobalAttributes::BugReportFile)
														 .arg(QDateTime::currentDateTime().toString(QString("_yyyyMMdd_hhmm"))))
									 .absoluteFilePath();

	//Opens the file for writting
	output.setFileName(filename);
	output.open(QFile::WriteOnly);

	if(!output.isOpen())
		msgbox.show(Exception::getErrorMessage(ErrorCode::FileDirectoryNotWritten).arg(filename), Messagebox::ErrorIcon);
	else
	{
		QByteArray comp_buf;

		//Compress the buffer (using zlib) in a compression rate at 8
		comp_buf=qCompress(buf, 8);

		//Saves the buffer
		output.write(comp_buf.data(), comp_buf.size());
		output.close();

		msgbox.show(tr("Bug report successfuly generated! Please, send the file <strong>%1</strong> to <em>%2</em> in order be analyzed. Thank you for the collaboration!")
								.arg(QDir::toNativeSeparators(filename)).arg(GlobalAttributes::BugReportEmail),
					Messagebox::InfoIcon);
	}
}

void BugReportForm::attachModel()
{
	QFileDialog file_dlg;

	try
	{
		file_dlg.setDefaultSuffix(QString("dbm"));
		file_dlg.setWindowTitle(tr("Load model"));
		file_dlg.setNameFilter(tr("Database model (*.dbm);;All files (*.*)"));
		file_dlg.setFileMode(QFileDialog::ExistingFile);
		file_dlg.setModal(true);

		if(file_dlg.exec()==QFileDialog::Accepted)
			attachModel(file_dlg.selectedFiles().at(0));
	}
	catch(Exception &e)
	{
		Messagebox msgbox;
		msgbox.show(e);
	}
}
