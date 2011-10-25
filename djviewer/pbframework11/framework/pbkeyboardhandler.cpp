#include "pbkeyboardhandler.h"
#include "inkview.h"
#include "inkinternal.h"

PBKeyboardHandler::PBKeyboardHandler()
{
    m_pStarter = NULL;
    m_keyReleased = true;
    m_keyMapping = true;
    m_keyMappingJoystick = true;
    m_mappingtype = KEYMAPPING_TXT;
    GetKeyMappingEx(m_mappingtype, m_keyact0, m_keyact1, 32);
}

PBKeyboardHandler::~PBKeyboardHandler()
{
}

int PBKeyboardHandler::registrationRunner(PBIEventHandler* pStarter)
{
    if (m_pStarter)
        return 0;

    m_pStarter = pStarter;
    return 1;
}

void PBKeyboardHandler::keyMapping(bool state)
{
    m_keyMapping = state;
}

void PBKeyboardHandler::keyMappingJoystick(bool state)
{
    m_keyMappingJoystick = state;
}

PBKeyboardHandler::KeyStates PBKeyboardHandler::GetKeyState(int message_type, int repeat_count)
{
    KeyStates result = stateNone;

    if (message_type == EVT_KEYPRESS)
    {
        m_keyReleased = false;
    }
    else if (!m_keyReleased && repeat_count >= 0)
    {
        if (message_type == EVT_KEYRELEASE)
        {
            if (repeat_count == 0)
            {
                result = statePress;
            }
            else
            {
                result = stateHoldOff;
            }
            m_keyReleased = true;
        }
        else if (message_type == EVT_KEYREPEAT)
        {
            result = stateHolding;
        }
    }
    return result;
}

char* PBKeyboardHandler::GetKeyMappingAction(int key_code, PBKeyboardHandler::KeyStates keyState)
{
    char* result = NULL;

	if (m_keyMapping == false) // key mapping is disabled
		return result;

	// keymapping for the joystick's keys may be disabled also
	// but only for the press event, not for hold or release
	if (m_keyMappingJoystick == false
		&& keyState == statePress
		&& (key_code == KEY_LEFT || key_code == KEY_RIGHT || key_code == KEY_UP || key_code == KEY_DOWN))
		return result;


	switch (keyState)
	{
	case statePress :
		result = m_keyact0[key_code];
		break;
	case stateHolding:
	case stateHoldOff:
		result = m_keyact1[key_code];
		break;
	default:
		break;
	}
	return result;
}

int PBKeyboardHandler::HandleEvent(int type, int par1, int par2)
{
	//fprintf(stderr, "PBKeyboardHandler::HandleEvent type=%d, par1=%d, par2=%d \n", type, par1, par2);

	int result = 0;

	if (par1 >= 0x20) // alpha numeric buttons
		return result;

	if (!m_pStarter)
	{
		fprintf(stderr, "WARNING: KeyBoard handler not installed.\n");
		return result;
	}

	KeyStates keyState = GetKeyState(type, par2);

	const char* mappingAction = GetKeyMappingAction(par1, statePress);
	const char* holdMappingAction = GetKeyMappingAction(par1, stateHolding);

	if (mappingAction == NULL && (keyState == stateNone || keyState == statePress) )
	{
		bool doAction = false;

		if ( IsNullAction(holdMappingAction) )
		{
			if (keyState == stateNone)
				doAction = true;
		}
		else if ( par1 == KEY_PREV || par1 == KEY_NEXT )
		{
			if (keyState == stateNone)
				doAction = true; // do action immidiately
		}
		else if (keyState == statePress)
		{
			// do action when the key is released
			doAction = true;
		}

		if (doAction)
			result = ProcessKeyPress(par1);
	}
	else if (keyState == stateNone || keyState == statePress)
	{
		bool doAction = false;

		if ( IsNullAction(holdMappingAction) )
		{
			if (keyState == stateNone)
				doAction = true;
		}
		else if ( IsTurnPageAction(mappingAction) && IsTurnPageAction(holdMappingAction) )
		{
			if (keyState == stateNone)
				doAction = true; // do action immidiately
		}
		else if (keyState == statePress)
		{
			// do action when the key is released
			doAction = true;
		}

		if (doAction)
		{
			fprintf(stderr, "PBKeyboardHandler::DoAction type=%d, par1=%d, par2=%d \n", type, par1, par2);
			result = DoAction(mappingAction, keyState);
		}
	}
	else if (keyState == stateHolding || keyState == stateHoldOff)
	{
		if ( !IsNullAction(holdMappingAction) )
			result = DoAction(holdMappingAction, keyState);
	}



/*
	if ((keyState = GetKeyState(type, par2)) != stateNone)
    {
		if (m_keyMapping)
			mappingAction = GetKeyMappingAction(par1, keyState);

		if ( mappingAction != NULL )
        {
			result = DoAction(mappingAction, keyState);
        }
        else
        {
            // mapping action not defined. Obtain some key exeptions
			iv_keyswitch_rtl(&par1);

            switch (par1)
            {
			case KEY_BACK:
				if (!m_pStarter->onExit())
					CloseApp();
				break;
			case KEY_LEFT:
				m_pStarter->onKeyLeft();
				break;
			case KEY_PREV:
				m_pStarter->onPagePrev();
				break;
			case KEY_RIGHT:
				m_pStarter->onKeyRight();
				break;
			case KEY_NEXT:
				m_pStarter->onPageNext();
				break;
			case KEY_UP:
				m_pStarter->onKeyUp();
				break;
			case KEY_DOWN:
				m_pStarter->onKeyDown();
				break;
			case KEY_OK:
				m_pStarter->onKeyOK();
				break;
			case KEY_MINUS:
				m_pStarter->onVolumeDown();
				break;
			case KEY_PLUS:
				m_pStarter->onVolumeUp();
				break;
			default:
				break;
            }
        }
	}*/
    return result;
}

void PBKeyboardHandler::setKeyboardMapping(int type) {

    m_mappingtype = type;
    GetKeyMappingEx(type, m_keyact0, m_keyact1, 32);

}

void PBKeyboardHandler::UpdateKeyMapping()
{
    GetKeyMappingEx(m_mappingtype, m_keyact0, m_keyact1, 32);
}

int PBKeyboardHandler::DoAction(const char* mappingAction, KeyStates keyState)
{
	//fprintf(stderr, "mapping - %s\n", mappingAction);
	int result = 1;

	if (!strcmp(mappingAction, "@KA_nx10"))
	{
		switch (keyState)
		{
		case statePress:
			m_pStarter->onPageJNext10();
			break;
		case stateHolding:
			m_pStarter->onPageJNext10Hold();
			break;
		case stateHoldOff:
			m_pStarter->onPageJNext10Up();
		default:
			break;
		}
	}
	else if (!strcmp(mappingAction, "@KA_pr10"))
	{
		switch (keyState)
		{
		case statePress:
			m_pStarter->onPageJPrev10();
			break;
		case stateHolding:
			m_pStarter->onPageJPrev10Hold();
			break;
		case stateHoldOff:
			m_pStarter->onPageJPrev10Up();
		default:
			break;
		}
	}
	else if (!strcmp(mappingAction, "@KA_goto"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onPageSelect();
	}
	else if (!strcmp(mappingAction, "@KA_frst"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onPageFirst();
	}
	else if (!strcmp(mappingAction, "@KA_last"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onPageLast();
	}
	else if (!strcmp(mappingAction, "@KA_prev"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onBack();
	}
	else if (!strcmp(mappingAction, "@KA_next"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onForward();
	}
	else if (!strcmp(mappingAction, "@KA_pgup"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onPagePrev();
	}
	else if (!strcmp(mappingAction, "@KA_pgdn"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onPageNext();
	}
	else if (!strcmp(mappingAction, "@KA_prse"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onPrevSelection();
	}
	else if (!strcmp(mappingAction, "@KA_nxse"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onNextSelection();
	}
	else if (!strcmp(mappingAction, "@KA_obmk"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onBookMarkOpen();
	}
	else if (!strcmp(mappingAction, "@KA_nbmk"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onBookMarkNew();
	}
	else if (!strcmp(mappingAction, "@KA_nnot"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onNoteNew();
	}
	else if (!strcmp(mappingAction, "@KA_onot"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onNoteOpen();
	}
	else if (!strcmp(mappingAction, "@KA_savp"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onNoteSave();
	}
	else if (!strcmp(mappingAction, "@KA_olnk"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onLinkOpen();
	}
	else if (!strcmp(mappingAction, "@KA_blnk"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onLinkBack();
	}
	else if (!strcmp(mappingAction, "@KA_cnts"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onContentsOpen();
	}
	else if (!strcmp(mappingAction, "@KA_srch"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onSearchStart();
	}
	else if (!strcmp(mappingAction, "@KA_dict"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onDictionaryOpen();
	}
	else if (!strcmp(mappingAction, "@KA_zmin"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onZoomIn();
	}
	else if (!strcmp(mappingAction, "@KA_zout"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onZoomOut();
	}
	else if (!strcmp(mappingAction, "@KA_zoom"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onZoomerOpen();
	}
	else if (!strcmp(mappingAction, "@KA_hidp"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onPanelHide();
	}
	else if (!strcmp(mappingAction, "@KA_rtte"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onRotateOpen();
	}
	else if (!strcmp(mappingAction, "@KA_mmnu"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onMainMenu();
	}
	else if (!strcmp(mappingAction, "@KA_exit"))
	{
		if (keyState != stateHoldOff)
			CloseApp();
	}
	else if (!strcmp(mappingAction, "@KA_mp3o"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onMp3Open();
	}
	else if (!strcmp(mappingAction, "@KA_mp3p"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onMp3Pause();
	}
	else if (!strcmp(mappingAction, "@KA_volp"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onVolumeUp();
	}
	else if (!strcmp(mappingAction, "@KA_volm"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onVolumeDown();
	}
	else if (!strcmp(mappingAction, "@KA_conf"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onConfigOpen();
	}
	else if (!strcmp(mappingAction, "@KA_abou"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onAboutOpen();
	}
	else if (!strcmp(mappingAction, "@KA_menu"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onMenu();
	}
	else if (!strcmp(mappingAction, "@KA_mpdf"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onPdfMode();
	}
	else if (!strcmp(mappingAction, "@KA_drpc"))
	{
		if (keyState != stateHoldOff)
			m_pStarter->onDropConnect();
	}
	else
	{
		result = 0;
		fprintf(stderr, "WARNING: Action not defined %s\n", mappingAction);
	}

	return result;
}

int PBKeyboardHandler::ProcessKeyPress(int key)
{
	int result  = 1;
	iv_keyswitch_rtl(&key);

	// mapping action not defined. Obtain some key exeptions
	switch (key)
	{
	case KEY_BACK:
		if (!m_pStarter->onExit())
			CloseApp();
		break;
	case KEY_LEFT:
		m_pStarter->onKeyLeft();
		break;
	case KEY_PREV:
		m_pStarter->onPagePrev();
		break;
	case KEY_RIGHT:
		m_pStarter->onKeyRight();
		break;
	case KEY_NEXT:
		m_pStarter->onPageNext();
		break;
	case KEY_UP:
		m_pStarter->onKeyUp();
		break;
	case KEY_DOWN:
		m_pStarter->onKeyDown();
		break;
	case KEY_OK:
		m_pStarter->onKeyOK();
		break;
	case KEY_MINUS:
		m_pStarter->onVolumeDown();
		break;
	case KEY_PLUS:
		m_pStarter->onVolumeUp();
		break;
	default:
		result = 0;
		break;
	}

	return result;
}

bool PBKeyboardHandler::IsTurnPageAction(const char* mappingAction)
{
	return ( strcmp(mappingAction, "@KA_prev") == 0 ||
			 strcmp(mappingAction, "@KA_next") == 0 ||
			 strcmp(mappingAction, "@KA_nx10") == 0 ||
			 strcmp(mappingAction, "@KA_pr10") == 0 );
}

bool PBKeyboardHandler::IsNullAction(const char* mappingAction)
{
	return mappingAction == NULL || strcmp(mappingAction, "@KA_none") == 0;
}
