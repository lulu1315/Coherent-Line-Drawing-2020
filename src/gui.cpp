#include "gui.h"
#include <ctime>
#include <fstream>
#include <iostream>

#define MODE_ORIGINIAL_IMAGE "Original Image"
#define MODE_ETF "ETF"
#define MODE_ETF_DEBUG "ETF-debug"
#define MODE_CLD "Coherent Line Drawing"
#define MODE_ANTI_ALIASING "Anti-Aliasing"

bool MyApp::OnInit() {
	MyFrame *frame = new MyFrame("Coherent Line Drawing | SSARCandy", wxPoint(50, 50), wxSize(1000, 730));
	//frame->Maximize(true);
	frame->Show(true);

	return true;
}

#pragma region MyFrame
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size) {
	ETF_kernel = 5;
	ETF_iteration = 0;
	FDoG_iteration = 0;

#pragma region MenuBar
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_ONOPENSRC, "&Open SrcImg\tCtrl-O", "Open source image");

	menuFile->Append(ID_ONSAVE, "&Save\tCtrl-S", "Save Result");
	menuFile->AppendSeparator();

	menuFile->Append(wxID_EXIT);

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT, "&About", "About the System");
	menuHelp->Append(new wxMenuItem(menuHelp, wxID_TOGGLE_LOG, wxString(wxT("&Log\tCtrl-L")), "Show/Hide the Log", wxITEM_CHECK))->Check(true);

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, "&File");
	menuBar->Append(menuHelp, "&Help");
	SetMenuBar(menuBar);
	CreateStatusBar(3);
	SetStatusText("SrcImg: None", 0);
	SetStatusText("ETF: None", 1);
	SetStatusText("FDoG: None", 2);

#pragma endregion

#pragma region ToolBar: Buttons(Start, Fill Ink, Clean), Combobox(processingBox)
	wxToolBar *toolbar1 = CreateToolBar();
	start = new wxButton(toolbar1, BUTTON_Start, _T("Start"), wxDefaultPosition, wxSize(100, 30), 0);
	clean = new wxButton(toolbar1, BUTTON_Clean, _T("Clean"), wxDefaultPosition, wxSize(100, 30), 0);

	processingBox = new wxComboBox(toolbar1, COMBOBOX_Processing, MODE_ORIGINIAL_IMAGE, wxDefaultPosition, wxSize(200, 30), 0);
	processingBox->Append(MODE_ORIGINIAL_IMAGE);
	processingBox->Append(MODE_ETF);
	processingBox->Append(MODE_ETF_DEBUG);
	processingBox->Append(MODE_CLD);
	processingBox->Append(MODE_ANTI_ALIASING);


	toolbar1->AddControl(start);
	toolbar1->AddControl(clean);
	toolbar1->AddControl(processingBox);


	toolbar1->Realize();
	SetToolBar(toolbar1);
#pragma endregion 

	//Sizer of whole window
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	//Sizer of leftside
	wxBoxSizer* leftside = new wxBoxSizer(wxVERTICAL);
	//Sizer of rightside(control panel)
	wxBoxSizer* rightside = new wxBoxSizer(wxVERTICAL);

	wxPanel* controlpanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	controlpanel->SetMaxSize(wxSize(300, -1));
	controlpanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));

	wxString s;

#pragma region Leftside: drawPane, log
	//drawing panel
	wxPanel *drawpanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	drawpanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));

	wxBoxSizer *dps = new wxBoxSizer(wxHORIZONTAL);
	dp = new wxPanel(drawpanel, wxID_ANY, wxDefaultPosition, wxSize(256, 256), wxTAB_TRAVERSAL);
	dp->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
	dp->SetSizer(dps);


	drawPane = new BasicDrawPane(dp, cv::Size(256, 256), true);
	dps->Add(drawPane, 1, wxEXPAND);

	// wxTextCtrl: http://docs.wxwidgets.org/trunk/classwx_text_ctrl.html
	log = new wxTextCtrl(drawpanel, ID_WXEDIT1, wxT(""), wxPoint(91, 43), wxSize(121, 21), wxTE_RICH2 | wxTE_MULTILINE | wxTE_READONLY, wxDefaultValidator, wxT("WxEdit1"));
	addlog("Hello Coherent Line Drawing!", wxColour(*wxBLACK));

	leftside->AddStretchSpacer(3);
	leftside->Add(dp, 0, wxCENTER);
	leftside->AddStretchSpacer(3);
	leftside->Add(log, 2, wxEXPAND);
#pragma endregion

#pragma region Paint Parameters
	//solve = new wxButton(controlpanel, BUTTON_SolveIt, wxT("Apply to CLD"), wxDefaultPosition, wxDefaultSize, 0);
	refineETF = new wxButton(controlpanel, BUTTON_RefineETF, wxT("Smooth ETF"), wxDefaultPosition, wxDefaultSize, 0);
	iterativeFDoG = new wxButton(controlpanel, BUTTON_IterativeFDoG, wxT("Iterative FDoG"), wxDefaultPosition, wxDefaultSize, 0);

	wxStaticBox *st_etf = new wxStaticBox(controlpanel, -1, wxT("ETF Parameters"), wxDefaultPosition, wxDefaultSize, wxTE_RICH2);
	wxStaticBoxSizer *st_etf_sizer = new wxStaticBoxSizer(st_etf, wxVERTICAL);

	s.Printf("ETF kernel size: %d", ETF_kernel);
	slider_ETFkernel_t = new wxStaticText(controlpanel, SLIDER_ETF_KERNEL_T, s, wxDefaultPosition, wxDefaultSize, 0);
	st_etf_sizer->Add(slider_ETFkernel_t, 0, wxEXPAND | wxLEFT, 10);
	slider_ETFkernel = new wxSlider(controlpanel, SLIDER_ETF_KERNEL, ETF_kernel, 3, 21, wxDefaultPosition, wxDefaultSize, 0);
	st_etf_sizer->Add(slider_ETFkernel, 0, wxEXPAND | wxLEFT, 10);

	wxStaticBox *st_paint = new wxStaticBox(controlpanel, -1, wxT("Line Parameters"), wxDefaultPosition, wxDefaultSize, wxTE_RICH2);
	wxStaticBoxSizer *st_paint_sizer = new wxStaticBoxSizer(st_paint, wxVERTICAL);

	s.Printf("Noise(rho) : %.3f", drawPane->cld.rho);
	slider_rho_t = new wxStaticText(controlpanel, SLIDER_RHO_T, s, wxDefaultPosition, wxDefaultSize, 0);
	st_paint_sizer->Add(slider_rho_t, 0, wxEXPAND | wxLEFT, 10);
	slider_rho = new wxSlider(controlpanel, SLIDER_RHO, int(drawPane->cld.rho*10000), 9000, 10000, wxDefaultPosition, wxDefaultSize, 0);
	st_paint_sizer->Add(slider_rho, 0, wxEXPAND | wxLEFT, 10);

	s.Printf("Degree of coherence(sigma_m): %.3f", drawPane->cld.sigma_m);
	slider_sigma1_t = new wxStaticText(controlpanel, SLIDER_SIGMA_M_T, s, wxDefaultPosition, wxDefaultSize, 0);
	st_paint_sizer->Add(slider_sigma1_t, 0, wxEXPAND | wxLEFT, 10);
	slider_sigma1 = new wxSlider(controlpanel, SLIDER_SIGMA_M, int(drawPane->cld.sigma_m * 1000), 10, 10000, wxDefaultPosition, wxDefaultSize, 0);
	st_paint_sizer->Add(slider_sigma1, 0, wxEXPAND | wxLEFT, 10);

	s.Printf("Line width(sigma_c): %.3f", drawPane->cld.sigma_c);
	slider_sigma2_t = new wxStaticText(controlpanel, SLIDER_SIGMA_C_T, s, wxDefaultPosition, wxDefaultSize, 0);
	st_paint_sizer->Add(slider_sigma2_t, 0, wxEXPAND | wxLEFT, 10);
	slider_sigma2 = new wxSlider(controlpanel, SLIDER_SIGMA_C, int(drawPane->cld.sigma_c * 1000), 10, 10000, wxDefaultPosition, wxDefaultSize, 0);
	st_paint_sizer->Add(slider_sigma2, 0, wxEXPAND | wxLEFT, 10);
	
	s.Printf("Thresholding(tau) : %.3f", drawPane->cld.tau);
	slider_t_t = new wxStaticText(controlpanel, SLIDER_TAU_T, s, wxDefaultPosition, wxDefaultSize, 0);
	st_paint_sizer->Add(slider_t_t, 0, wxEXPAND | wxLEFT, 10);
	slider_t = new wxSlider(controlpanel, SLIDER_TAU, int(drawPane->cld.tau * 10000), 4000, 10000, wxDefaultPosition, wxDefaultSize, 0);
	st_paint_sizer->Add(slider_t, 0, wxEXPAND | wxLEFT, 10);



	rightside->Add(st_etf_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 3);
	rightside->Add(refineETF, 0, wxEXPAND | wxALL, 10);

	rightside->Add(st_paint_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 3);
	rightside->Add(iterativeFDoG, 0, wxEXPAND | wxALL, 10);
	//rightside->Add(solve, 0, wxEXPAND | wxALL, 10);
#pragma endregion


	//set portion of size: leftside & rightside(control)
	drawpanel->SetSizer(leftside);
	controlpanel->SetSizer(rightside);
	sizer->Add(drawpanel, 7, wxEXPAND);
	sizer->Add(controlpanel, 3, wxEXPAND);
	rightside->Fit(controlpanel);
	leftside->Fit(drawpanel);
	SetSizer(sizer);

	this->GetSizer()->Layout();
	render_loop_on = false;
}
void MyFrame::OnExit(wxCommandEvent& event) {
	Close(true);
}
void MyFrame::OnAbout(wxCommandEvent& event) {
	wxMessageBox(
		"Coherent Line Drawing\n\nProgramed by HSU,SHU-HSUAN(National Taiwan University)\n\nThis is an implementation of 'Coherent Line Drawing' by Kang et al, Proc. NPAR 2007 .",
		"About Coherent Line Drawing",
		wxOK | wxICON_INFORMATION
	);
}
void MyFrame::OnToggleLog(wxCommandEvent& event) {
	event.IsChecked() ? log->Show() : log->Hide();

	this->GetSizer()->Layout();
}

void MyFrame::OnOpenSrc(wxCommandEvent& event) {
	render_loop_on = false;
	activateRenderLoop(render_loop_on);
	wxFileDialog openFileDialog(this, _("Open image file"), "", "", "image files (*.bmp;*.png;*.jpg)|*.bmp;*.png;*.jpg", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	wxString s;

	if (openFileDialog.ShowModal() == wxID_CANCEL) {
		addlog("Load Img Canceled", wxColour(*wxBLACK));
		return;     // the user changed idea...
	}
	else {
		processingBox->SetValue(MODE_ORIGINIAL_IMAGE);
		drawPane->processingS = MODE_ORIGINIAL_IMAGE;

		s.Printf("Load Img - %s", openFileDialog.GetFilename());
		addlog(s, wxColour(*wxBLUE));

		s.Printf("SrcImg: %s", openFileDialog.GetFilename());
		SetStatusText(s, 0);
	}

	// proceed loading the file chosen by the user, this can be done with e.g. wxWidgets input streams:
	wxFileInputStream input_stream(openFileDialog.GetPath());
	if (!input_stream.IsOk()) {
		wxLogError("Cannot open file '%s'.", openFileDialog.GetPath());
		return;
	}
	drawPane->cld.readSrc((const char*)openFileDialog.GetPath().mb_str());

	wxSize img(drawPane->cld.originalImg.cols, drawPane->cld.originalImg.rows);
	dp->SetMinSize(img);
	this->Layout();

	render_loop_on = true;
	activateRenderLoop(render_loop_on);
	ETF_iteration = FDoG_iteration = 0;
	s.Printf("ETF: %d iterations", ETF_iteration);
	SetStatusText(s, 1);
}

void MyFrame::OnSaveResult(wxCommandEvent& event) {
	wxFileDialog
		saveFileDialog(this, _("Save PNG file"), "", "",
		"PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (saveFileDialog.ShowModal() == wxID_CANCEL)
		return;     // the user changed idea...

	// save the current contents in the file;
	// this can be done with e.g. wxWidgets output streams:
	wxFileOutputStream output_stream(saveFileDialog.GetPath());
	if (!output_stream.IsOk()) {
		wxLogError("Cannot save current contents in file '%s'.", saveFileDialog.GetPath());
		return;
	}

	cv::cvtColor(drawPane->dis, drawPane->dis, cv::COLOR_BGR2RGB);
	cv::imwrite((const char*)saveFileDialog.GetPath().mb_str(), drawPane->dis);
}


//Buttons
void MyFrame::OnStart(wxCommandEvent& event) {
	render_loop_on = !render_loop_on;
	activateRenderLoop(render_loop_on);
}

void MyFrame::OnClean(wxCommandEvent& event) {
	drawPane->cld.init(cv::Size(300, 300));
	ETF_iteration = FDoG_iteration = 0;
	drawPane->paintNow(true); //execute clean action

	wxSize img(drawPane->cld.originalImg.cols, drawPane->cld.originalImg.rows);
	dp->SetMinSize(img);
	this->Layout();

	addlog("Draw Panel Cleaned.", wxColour(*wxRED));
	
	wxString s;
	s.Printf("SrcImg: None");
	SetStatusText(s, 0);
	s.Printf("ETF: None");
	SetStatusText(s, 1);
	s.Printf("FDoG: None");
	SetStatusText(s, 2);
}

void MyFrame::OnSolveIt(wxCommandEvent& event) {
	addlog("[CLD] Computing CLD...", wxColour(*wxBLUE));
	drawPane->cld.genCLD();
	addlog("[CLD] done", wxColour(*wxBLUE));
}

void MyFrame::OnRefineETF(wxCommandEvent& event) {
	addlog("[ETF] Refining ETF...", wxColour(*wxBLUE));
	(drawPane->cld).etf.refine_ETF(ETF_kernel);
	addlog("[ETF] Done", wxColour(*wxBLUE));

	wxString s;
	s.Printf("ETF: %d iterations", ++ETF_iteration);
	SetStatusText(s, 1);
}

void MyFrame::OnIterativeFDoG(wxCommandEvent& event) {
	addlog("[CLD] Iterative FDoG...", wxColour(*wxBLUE));
	(drawPane->cld).combineImage();
	(drawPane->cld).genCLD();
	addlog("[CLD] Done", wxColour(*wxBLUE));

	wxString s;
	s.Printf("FDoG: %d iterations", ++FDoG_iteration);
	SetStatusText(s, 2);
	drawPane->paintNow(true);
}

//Comboboxes
void MyFrame::OnProcessingBox(wxCommandEvent& event) {
	wxString s = processingBox->GetValue();
	drawPane->processingS = s;

	if (s == MODE_ETF || s == MODE_ETF_DEBUG) {
		render_loop_on = true;
	} else {
		render_loop_on = false;
	}

	if (s == MODE_CLD) {
		drawPane->cld.genCLD();
		wxString s;
		s.Printf("FDoG: %d iterations", FDoG_iteration);
		SetStatusText(s, 2);
	}

	addlog("[Mode Changed] " + s, wxColour(*wxBLACK));
	drawPane->paintNow(true);
	activateRenderLoop(render_loop_on);
}

//Slides: Pattern Parameter
void MyFrame::OnSliderETFkernel(wxCommandEvent& event) {
	wxString s;
	ETF_kernel = slider_ETFkernel->GetValue();
	s.Printf("ETF kernel size : %d", ETF_kernel);
	slider_ETFkernel_t->SetLabel(s);

	//drawPane->cld.genCLD();
}

void MyFrame::OnSliderRho(wxCommandEvent& event) {
	wxString s;
	drawPane->cld.rho = slider_rho->GetValue() / 10000.0;
	s.Printf("Noise(rho) : %.3f", drawPane->cld.rho);
	slider_rho_t->SetLabel(s);

	drawPane->cld.genCLD();
	drawPane->paintNow(true); //execute action
}

void MyFrame::OnSliderSigmaM(wxCommandEvent& event) {
	wxString s;
	drawPane->cld.sigma_m = slider_sigma1->GetValue() / 1000.0;
	s.Printf("Degree of coherence(sigma_m): %.3f", drawPane->cld.sigma_m);
	slider_sigma1_t->SetLabel(s);

	drawPane->cld.genCLD();
	drawPane->paintNow(true); //execute action
}

void MyFrame::OnSliderSigmaC(wxCommandEvent& event) {
	wxString s;
	drawPane->cld.sigma_c = slider_sigma2->GetValue() / 1000.0;
	s.Printf("Line width(sigma_c): %.3f", drawPane->cld.sigma_c);
	slider_sigma2_t->SetLabel(s);

	drawPane->cld.genCLD();
	drawPane->paintNow(true); //execute action
}

void MyFrame::OnSliderTau(wxCommandEvent& event) {
	drawPane->cld.tau = slider_t->GetValue() / 10000.0;
	wxString s;
	s.Printf("Thresholding(Tau) : %.3f", drawPane->cld.tau);
	slider_t_t->SetLabel(s);

	drawPane->cld.binaryThresholding(drawPane->cld.FDoG, drawPane->cld.result, drawPane->cld.tau);
	drawPane->paintNow(true); //execute action
}

void MyFrame::addlog(wxString info, const wxColour& color) {
	time_t currentTime;// for logging current time
	struct tm *localTime;// for logging current time
	time(&currentTime);                   // Get the current time
	localTime = localtime(&currentTime);  // Convert the current time to the local time
	wxString s;
	s.Printf("\n%02d : %02d : %02d  |  %s", localTime->tm_hour, localTime->tm_min, localTime->tm_sec, info);
	log->SetDefaultStyle(wxTextAttr(color));
	log->AppendText(s);

}
void MyFrame::activateRenderLoop(bool on) {
	if (on) {
		start->SetLabel("Stop");
		Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(MyFrame::onIdle));
		render_loop_on = true;
		addlog("-------Start iteration-------", wxColour(*wxBLACK));
	}
	else {
		start->SetLabel("Start");
		Disconnect(wxEVT_IDLE, wxIdleEventHandler(MyFrame::onIdle));
		//Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(MyFrame::onIdle));
		render_loop_on = false;
		addlog("-------Stop iteration-------", wxColour(*wxBLACK));
	}
}
void MyFrame::onIdle(wxIdleEvent& evt) {
	drawPane->paintNow(render_loop_on);
	evt.RequestMore(); // render continuously, not only once on idle
}
#pragma endregion 

#pragma region BasicDrawPane
BasicDrawPane::BasicDrawPane(wxPanel* parent, cv::Size s, bool canUndo) :
processing(s),
cld(s),
wxPanel(parent) {
	activateDraw = false;
}


//first frame
void BasicDrawPane::paintEvent(wxPaintEvent& evt) {
	wxPaintDC dc(this);
	//render(dc);
	dis = cld.originalImg.clone();
	dis.convertTo(dis, CV_8UC1, 255);
	//cvtColor(dis, dis, CV_GRAY2RGB);
	wxImage img(dis.cols, dis.rows, dis.data, true);
	wxBitmap bmp(img);
	dc.DrawBitmap(bmp, 0, 0);
}

//render loop
void BasicDrawPane::paintNow(bool render_loop_on) {
	wxClientDC dc(this);
	render(dc, render_loop_on);
}

//Main Render(iteration) Section
void BasicDrawPane::render(wxDC& dc, bool render_loop_on) {
	dis = cld.originalImg.clone();
	cv::cvtColor(dis, dis, cv::COLOR_GRAY2BGR);

	if (processingS == MODE_ETF) {
		processing.ETF(cld.etf.flowField, dis,10,2);
		dis.convertTo(dis, CV_8UC1, 255);
		cv::cvtColor(dis, dis, cv::COLOR_GRAY2BGR);
	}
	else if (processingS == MODE_ETF_DEBUG) {
		processing.FlowField(cld.etf.flowField, dis);
	}
	else if (processingS == MODE_CLD) {
		dis = cld.result.clone();
		cv::cvtColor(dis, dis, cv::COLOR_GRAY2BGR);
	}
	else if (processingS == MODE_ANTI_ALIASING) {
		dis = cld.result.clone();
		processing.AntiAlias(dis, dis);
		cv::cvtColor(dis, dis, cv::COLOR_GRAY2BGR);
	}

	wxImage img(dis.cols, dis.rows, dis.data, true);
	wxBitmap bmp(img);
	dc.DrawBitmap(bmp, 0, 0);
}
#pragma endregion 
