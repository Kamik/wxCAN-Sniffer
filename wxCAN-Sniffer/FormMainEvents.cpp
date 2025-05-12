#include "FormMain.h"

// ������� �������
wxBEGIN_EVENT_TABLE(FormMain, wxFrame)
wxEND_EVENT_TABLE()

// ������� �� �������� ������ ����������������� �����
wxDEFINE_EVENT(wxEVT_SERIAL_PORT_THREAD_STARTED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SERIAL_PORT_THREAD_UPDATE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SERIAL_PORT_THREAD_EXIT, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_SERIAL_PORT_THREAD_MESSAGE, wxThreadEvent);

// ����������� ������� �������� ����
void FormMain::AssignEventHandlers()
{
    // �������� �������
    this->Bind(wxEVT_CLOSE_WINDOW, &FormMain::OnClose, this, IDs::MAIN_FORM);
    choiceMode->Bind(wxEVT_CHOICE, &FormMain::ChoiceMode_OnChoice, this);
    choiceCANSpeed->Bind(wxEVT_CHOICE, &FormMain::ChoiceCANSpeed_OnChoice, this);
    buttonConnectDisconnect->Bind(wxEVT_BUTTON, &FormMain::ButtonConnectDisconnect_OnClick, this);
    buttonAdd->Bind(wxEVT_BUTTON, &FormMain::ButtonAdd_OnClick, this);
    buttonRemove->Bind(wxEVT_BUTTON, &FormMain::ButtonRemove_OnClick, this);
    buttonRemoveAll->Bind(wxEVT_BUTTON, &FormMain::ButtonRemoveAll_OnClick, this);
    buttonSend->Bind(wxEVT_BUTTON, &FormMain::ButtonSend_OnClick, this);
    buttonClearCANLog->Bind(wxEVT_BUTTON, &FormMain::ButtonClearCANLog_OnClick, this);
    gridCANView->Bind(wxEVT_GRID_SELECT_CELL, &FormMain::GridCANView_OnSelectCell, this);
    checkLogEnable->Bind(wxEVT_CHECKBOX, &FormMain::CheckLogEnable_OnClick, this);
    choiceExt->Bind(wxEVT_CHOICE, &FormMain::ChoiceExt_OnChoice, this);
    choiceSep->Bind(wxEVT_CHOICE, &FormMain::ChoiceSep_OnChoice, this);
    checkDec->Bind(wxEVT_CHECKBOX, &FormMain::CheckDec_OnClick, this);
    checkSingle->Bind(wxEVT_CHECKBOX, &FormMain::CheckSingle_OnClick, this);
    choiceDataType->Bind(wxEVT_CHOICE, &FormMain::ChoiceDataType_OnChoice, this);
    checkEndian->Bind(wxEVT_CHECKBOX, &FormMain::CheckEndian_OnClick, this);
    checkASCII->Bind(wxEVT_CHECKBOX, &FormMain::CheckASCII_OnClick, this);
    textDecWordMul->Bind(wxEVT_TEXT_ENTER, &FormMain::TextDecWordMul_OnEnter, this);
    textCANAnswerID->Bind(wxEVT_TEXT_ENTER, &FormMain::TextCANAnswerID_OnEnter, this);

    // ������� ������ �������
    drawPanel->Bind(wxEVT_PAINT, &FormMain::DrawPanel_OnPaint, this);
    drawPanel->Bind(wxEVT_ERASE_BACKGROUND, &FormMain::DrawPanel_OnEraseBackground, this);

    // ������� �� �������� ������ COM-�����
    this->Bind(wxEVT_SERIAL_PORT_THREAD_STARTED, &FormMain::Thread_OnStarted, this);
    this->Bind(wxEVT_SERIAL_PORT_THREAD_UPDATE, &FormMain::Thread_OnUpdate, this);
    this->Bind(wxEVT_SERIAL_PORT_THREAD_EXIT, &FormMain::Thread_OnExit, this);
    this->Bind(wxEVT_SERIAL_PORT_THREAD_MESSAGE, &FormMain::Thread_OnMessage, this);

    // ������� ������ �� UDP-������
    this->Bind(wxEVT_SOCKET, &FormMain::UDPSocket_OnEvent, this, IDs::UDP_SOCKET);

    // ������� �������
    this->Bind(wxEVT_TIMER, &FormMain::MainTimer_OnTimer, this, IDs::MAIN_TIMER);
}

// ������������ �������� ������� - ��������� ������
void FormMain::MainTimer_OnTimer(wxTimerEvent& event)
{
    // �������� ��������� ����������������� �����
    if (mode == Modes::Serial && serialPort && serialPort->hSerial == INVALID_HANDLE_VALUE)
    {
        delete serialPort;
        serialPort = nullptr;
        buttonConnectDisconnect->SetLabelText(TEXT_CONNECT);
        EnableConnectionControls();
    }

    // ��������� ���� ������� �� "����������"
    frames->ProcessAllFrames();

    if (screenUpdateCounter >= SCREEN_UPDATE_COUNTER_LIMIT)
    {
        // �������� ������ � �������
        RefreshGridCANView();

        ShowNumbers();

        // ��� ������� ������� OnPaint ��� ������
        drawPanel->Refresh();
        screenUpdateCounter = 0;
    }

    screenUpdateCounter++;
}

// ����� ������ ������
void FormMain::ChoiceMode_OnChoice(wxCommandEvent& event)
{
    mode = (Modes)event.GetSelection();
    switch (mode)
    {
        case Modes::Serial:
            comboBoxSerialPort->Show(true);
            comboBoxIPAddress->Show(false);
            break;

        case Modes::Network:
            comboBoxIPAddress->Show(true);
            comboBoxSerialPort->Show(false);
            break;
    }
    Layout();
}

// ����� �������� CAN-����
void FormMain::ChoiceCANSpeed_OnChoice(wxCommandEvent& event)
{
    uint32_t value;
    if (event.GetString().ToUInt(&value))
    {
        selectedCanSpeed = (uint16_t)value;
    }
    else
    {
        selectedCanSpeed = 500;
    }
}

// ���������� ������ ����������/���������
void FormMain::ButtonConnectDisconnect_OnClick(wxCommandEvent& event)
{
    rowToView = -1;
    colToView = -1;
    drawData->Clear();

    switch (mode)
    {
        // �����: ���������������� ����
        case Modes::Serial:
            try
            {
                // ���� ���� �� ������ - �������, ����� - �������
                if (serialPort == nullptr)
                {
                    DisableConnectionControls();
                    frames->Clear();

                    // ������� ��� ������ �������
                    if (gridCANView->GetNumberRows() > 0)
                    {
                        gridCANView->DeleteRows(0, gridCANView->GetNumberRows());
                    }

                    serialPort = new ThreadedSerialPort(comboBoxSerialPort->GetValue(), Parameters::serial.PortSpeed, (wxFrame*)this);
                    // ����� �������� � ���������, ��� ��������� �������� ��� ��������� � ������� ������� MainTimer_OnTimer
                    buttonConnectDisconnect->SetLabelText(TEXT_DISCONNECT);
                    // ������� ����������� ����� ���������� � ������� ������� ������ Thread_OnStarted
                }
                else
                {
                    // ��������� ����������� ����� ����������
                    SendCANCommand(CANCommands::Disconnect);
                    // ������ ��������� ������� ��������� ������, ��� ��������� �������� ��� ��������� � ������� ������� MainTimer_OnTimer
                    serialPort->Delete();
                    // �������� ��� log-�����
                    FlushLogs();
                    logFiles.clear();
                    EnableConnectionControls();
                }
            }
            catch (...)
            {
                buttonConnectDisconnect->SetLabelText(TEXT_CONNECT);
                EnableConnectionControls();
                wxMessageBox(ERROR_SERIAL);
            }
            break;
            
        // �����: ����
        case Modes::Network:
            try
            {
                // �������� ������� �����������/����������, ���� ���� ������� ����������
                if (udpSocket)
                {
                    if (!udpConnected)
                    {
                        SendCANCommand(CANCommands::Connect, selectedCanSpeed);
                        buttonConnectDisconnect->SetLabelText(TEXT_DISCONNECT);
                        DisableConnectionControls();
                    }
                    else
                    {
                        SendCANCommand(CANCommands::Disconnect);
                        buttonConnectDisconnect->SetLabelText(TEXT_CONNECT);
                        EnableConnectionControls();
                    }
                    udpConnected = !udpConnected;
                }
            }
            catch (...)
            {
                buttonConnectDisconnect->SetLabelText(TEXT_CONNECT);
                EnableConnectionControls();
            }
            break;
    }
}

// ����� ������ � ���������������� ������ ����������
void FormMain::Thread_OnStarted(wxThreadEvent& event)
{
    // ��������� ����������� ����� �����������
    SendCANCommand(CANCommands::Connect, selectedCanSpeed);
}

// �� ������� �� ������ �������� ��� �������� CAN-������, ������� ���� � ������
void FormMain::Thread_OnUpdate(wxThreadEvent& event)
{
    CANFrameIn frame;

    if (serialPort)
    {
        while (serialPort->GetNextFrame(frame))
        {
            ProcessCANFrame(frame);
        }
    }
}

// �� ������� �� ������ �������� ��� �������� CAN-������, ������� ���� � ������
void FormMain::Thread_OnExit(wxThreadEvent& event)
{
    if (serialPort)
    {
        serialPort = nullptr;
    }
    buttonConnectDisconnect->SetLabelText(TEXT_CONNECT);
    EnableConnectionControls();
}

// ��������� �� ������ ��� ������
void FormMain::Thread_OnMessage(wxThreadEvent& event)
{
    wxMessageBox(event.GetPayload<wxString>(), ERROR_CAPTION, wxICON_ERROR);
}

// ������ ������ ��� ������ ��� �����������
void FormMain::ChoiceDataType_OnChoice(wxCommandEvent& event)
{
    dataType = (DataTypes)event.GetSelection();
    checkEndian->Enable(dataType != DataTypes::UInt8 && dataType != DataTypes::Int8);
}

// ��� ������� Enter � ���� ����� ��������� - ���������� ��� � ���������
void FormMain::TextDecWordMul_OnEnter(wxCommandEvent& event)
{
    double newMul;
    if (textDecWordMul->GetValue().ToDouble(&newMul))
    {
        float newFloatMul = (float)newMul;
        if (newFloatMul == 0)
            mul = 1.0;
        else if (newFloatMul > 10000)
            mul = 10000.0;
        else
            mul = newFloatMul;
    }

    textDecWordMul->ChangeValue(wxString::Format(FORMAT_FLOAT1_3, mul));
}

// �������� ID � ������ ������� ��� ������ � log
void FormMain::ButtonAdd_OnClick(wxCommandEvent& event)
{
    if (rowToLog >= 0 && (int)frames->Size() > rowToLog)
    {
        bool found = false;
        for (size_t iID = 0; iID < logFilterIDs.size(); iID++)
        {
            if (frames->GetFrame(rowToLog).frame.id == logFilterIDs[iID])
            {
                found = true;
                break;
            }
        }
        if (!found)
            logFilterIDs.push_back(frames->GetFrame(rowToLog).frame.id);
    }

    RefreshListLog();
}

// ������� ID �� ������ ������� ������ � log
void FormMain::ButtonRemove_OnClick(wxCommandEvent& event)
{
    int32_t index = listLog->GetSelection();

    if (index > -1)
    {
        logFilterIDs.erase(logFilterIDs.begin() + index);
    }

    RefreshListLog();
}

// �������� ���� ������ ������� ������ � log
void FormMain::ButtonRemoveAll_OnClick(wxCommandEvent& event)
{
    logFilterIDs.clear();
    RefreshListLog();
}

// ����� ���������� ������ � ������
void FormMain::CheckLogEnable_OnClick(wxCommandEvent& event)
{
    logEnable = event.IsChecked();
}

// ����� ���������� �����
void FormMain::ChoiceExt_OnChoice(wxCommandEvent& event)
{
    logExt = event.GetString();
}

// ����� ����������� � log-�����
void FormMain::ChoiceSep_OnChoice(wxCommandEvent& event)
{
    auto value = event.GetString();
    if (value == TEXT_LOG_SEPARATOR_TAB)
        logSeparator = TEXT_LOG_SEPARATOR_TAB_VALUE;
    else if (value == TEXT_LOG_SEPARATOR_SPACE)
        logSeparator = TEXT_LOG_SEPARATOR_SPACE_VALUE;
    else
        logSeparator = value;
}

// ���� �� ���������� ����� ������ log
void FormMain::CheckDec_OnClick(wxCommandEvent& event)
{
    logDecimal = event.IsChecked();
}

// ���� �� ������ � ���� ����
void FormMain::CheckSingle_OnClick(wxCommandEvent& event)
{
    logSingle = event.IsChecked();
}

// ���� ��� ���������� � log-���� ASCII ������
void FormMain::CheckASCII_OnClick(wxCommandEvent& event)
{
    logASCII = event.IsChecked();
}

// ���� �� ������ big endian ��� little endian
void FormMain::CheckEndian_OnClick(wxCommandEvent& event)
{
    bigEndian = event.IsChecked();
}

// ����� ������ � �������
void FormMain::GridCANView_OnSelectCell(wxGridEvent& event)
{
    // ��������� ������ ��� ���������� � log
    rowToLog = event.GetRow();

    rowToView = rowToLog;
    colToView = event.GetCol() - 3;
    // ���� ������ ������� �� � �������
    if (colToView < 0)
    {
        rowToView = -1;
        colToView = -1;

        textBin->ChangeValue(wxEmptyString);
        textDec->ChangeValue(wxEmptyString);
        textDecimalResult->ChangeValue(wxEmptyString);
    }
    else
    {
        // �������� ����� � ������ ��������
        ShowNumbers();
    }
}

// ������� ������ ������� ������ �� ������������ �����
void FormMain::ButtonClearCANLog_OnClick(wxCommandEvent& event)
{
    // ������� ��� ������ �������
    if (gridCANLog->GetNumberRows() > 0)
    {
        gridCANLog->DeleteRows(0, gridCANLog->GetNumberRows());
    }
}

// ������� ������ �������� CAN-������
void FormMain::ButtonSend_OnClick(wxCommandEvent& event)
{
    unsigned long tempValue;

    // ������� CAN-����� ��� ��������
    CANFrameOut frame = { 0 };

    // ID ������
    textCANID->GetValue().ToULong(&tempValue, 16);
    if (tempValue > 0)
        frame.id = (uint32_t)tempValue;
    else
        return;
    // ���������������
    frame.reserved = 0;
    // ����� ������ ������
    textCANLength->GetValue().ToULong(&tempValue, 10);
    if (tempValue >= 0 && tempValue <= 8)
        frame.length = (uint8_t)tempValue;
    else
        return;
    // ���� 1
    textCANByte0->GetValue().ToULong(&tempValue, 16);
    if (tempValue >= 0 && tempValue <= 0xFF)
        frame.data[0] = (uint8_t)tempValue;
    else
        return;
    // ���� 2
    textCANByte1->GetValue().ToULong(&tempValue, 16);
    if (tempValue >= 0 && tempValue <= 0xFF)
        frame.data[1] = (uint8_t)tempValue;
    else
        return;
    // ���� 3
    textCANByte2->GetValue().ToULong(&tempValue, 16);
    if (tempValue >= 0 && tempValue <= 0xFF)
        frame.data[2] = (uint8_t)tempValue;
    else
        return;
    // ���� 4
    textCANByte3->GetValue().ToULong(&tempValue, 16);
    if (tempValue >= 0 && tempValue <= 0xFF)
        frame.data[3] = (uint8_t)tempValue;
    else
        return;
    // ���� 5
    textCANByte4->GetValue().ToULong(&tempValue, 16);
    if (tempValue >= 0 && tempValue <= 0xFF)
        frame.data[4] = (uint8_t)tempValue;
    else
        return;
    // ���� 6
    textCANByte5->GetValue().ToULong(&tempValue, 16);
    if (tempValue >= 0 && tempValue <= 0xFF)
        frame.data[5] = (uint8_t)tempValue;
    else
        return;
    // ���� 7
    textCANByte6->GetValue().ToULong(&tempValue, 16);
    if (tempValue >= 0 && tempValue <= 0xFF)
        frame.data[6] = (uint8_t)tempValue;
    else
        return;
    // ���� 8
    textCANByte7->GetValue().ToULong(&tempValue, 16);
    if (tempValue >= 0 && tempValue <= 0xFF)
        frame.data[7] = (uint8_t)tempValue;
    else
        return;
    // ��������� ID ������, �� �������� ��������� �����
    textCANAnswerID->GetValue().ToULong(&tempValue, 16);
    if (tempValue > 0)
        answerID = (uint32_t)tempValue;
    else
        answerID = 0;

    // ��������� ������
    if (serialPort)
    {
        serialPort->SendFrame(frame);
    }
    if (udpSocket)
    {
        UDPSocket_SendFrame(frame);
    }
}

// ������� Enter ��� ����� ������ CAN ID �������� ������
void FormMain::TextCANAnswerID_OnEnter(wxCommandEvent& event)
{
    unsigned long value;
    textCANAnswerID->GetValue().ToCULong(&value, 16);
    answerID = (uint32_t)value;
    textCANAnswerID->ChangeValue(wxString::Format(FORMAT_HEX3, answerID));
}

// ������� UDP-������
void FormMain::UDPSocket_OnEvent(wxSocketEvent& event)
{
    uint8_t  receivedData[UDP_BUFFER_SIZE] = { 0 };
    uint8_t* receivedDataPointer = receivedData;

    if (event.GetSocketEvent() == wxSOCKET_INPUT)
    {
        size_t receivedDataLen = udpSocket->RecvFrom(mcIpAddress, receivedData, UDP_BUFFER_SIZE).LastCount();
        if (receivedDataLen)
        {
            CANFrameIn frame;
            uint8_t* receivedDataTail = receivedDataPointer + receivedDataLen;
            // ����� CAN-������ � ������������ ������
            while (receivedDataPointer < receivedDataTail)
            {
                // ����� ������ - ���������� �����
                if (CANParser::Parse(&receivedDataPointer, frame))
                {
                    ProcessCANFrame(frame);
                }
            }
        }
    }
}

// ������� ��������� � ������ �������
void FormMain::DrawPanel_OnPaint(wxPaintEvent& event)
{
    //wxPaintDC dc(drawPanel);
    //wxBufferedPaintDC dc(drawPanel);
    wxAutoBufferedPaintDC dc(drawPanel);
    PrepareDC(dc);
    auto drawRectangle = drawPanel->GetClientRect();

    // ���������� ����� � ���
    dc.SetPen(graphFramePen);
    dc.SetBrush(graphBackgroundBrush);
    dc.DrawRectangle(drawRectangle);

    if (drawData && colToView >= 0)
    {
        float* frame = drawData->Frame();

        float minValue = 0;
        float maxValue = 0;
        // ����� ����������� �������� ��� �������
        for (size_t index = 0; index < drawFrameSize; index++)
        {
            float nextValue = *(frame + index);
            if (nextValue < minValue) minValue = nextValue;
            if (nextValue > maxValue) maxValue = nextValue;
        }

        // ���������� ����������� ������������
        float height = (float)drawRectangle.height;
        float scaleFactor = -height / (maxValue - minValue);

        // ���������� ������ ����� �� ����
        dc.SetPen(Parameters::colors.GraphText);
        wxCoord y = (-minValue) * scaleFactor + height;
        dc.DrawLine(0, y, drawRectangle.width, y);

        // �������� ������������������ ������
        dc.SetPen(graphPen);
        y = (*frame - minValue) * scaleFactor + height;
        for (size_t index = 1; index < drawFrameSize; index++)
        {
            wxCoord yy = (*(frame + index) - minValue) * scaleFactor + height;
            dc.DrawLine(index - 1, y, index, yy);
            y = yy;
        }

        dc.SetTextForeground(Parameters::colors.GraphText);
        auto fontMetrics = dc.GetFontMetrics();
        dc.DrawText(wxString::Format(FORMAT_FLOAT1_0, maxValue), fontMetrics.internalLeading, 0);
        dc.DrawText(wxString::Format(FORMAT_FLOAT1_0, minValue), fontMetrics.internalLeading, drawRectangle.height - fontMetrics.height - fontMetrics.descent);
    }
}

// �������� ���� ������ �������
void FormMain::DrawPanel_OnEraseBackground(wxEraseEvent& event)
{
    // ���� ���, ������������ ��� ��������
}
