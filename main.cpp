#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#include <iostream>
#include <windows.h>
#include <string>
#include <initguid.h>
#include <KnownFolders.h>
#include <shlobj.h>
using namespace std;

void (*NtSetTimerResolution)(ULONG, bool, PULONG) = 0;
//the timings are set in such a way as to make sure that there's at least one timing interval between every shot with every gun in apex legends (this is correct as of season 13s patch)
double timingsoriginal[66] = {0.027777500000, 0.091666666667, 0.138590129032, 0.180107526881, 0.267919758065, 0.287035500000, 0.351851666666, 0.430553333333, 0.480766730769, 0.528489769230, 0.568100358423, 0.630272952853, 0.679487179487, 0.759615384613, 0.788888888888, 0.867516923077, 0.933755961538, 0.983865967742, 1.016129032258, 1.066234038461, 1.132478632478, 1.211105000000, 1.240384615382, 1.320506153846, 1.369727047146, 1.431892419355, 1.519230769230, 1.569444444440, 1.648147333330, 1.726850999997, 1.816666666663, 1.870369444443, 1.962961000000, 2.041666666663, 2.138888888883, 2.183333333329, 2.274999999995, 2.366666666666, 2.472222222221, 2.527777777777, 2.633333333332, 2.738888888887, 2.844444444443, 2.949999999999, 3.055555555553, 3.166666666663, 3.277777777775, 3.388888888885, 3.499999999996, 3.611111111108, 3.722222222218, 3.833333333329, 3.944444444441, 4.055555555551, 4.166666666663, 4.277777777773, 4.388888888884, 4.499999999996, 4.611111111106, 4.722222222217, 4.833333333329, 4.944444444439, 5.055555555550, 5.166666666662, 5.277777777772, 5.388888888883};
double timings[66] = {0.027777500000, 0.091666666667, 0.138590129032, 0.180107526881, 0.267919758065, 0.287035500000, 0.351851666666, 0.430553333333, 0.480766730769, 0.528489769230, 0.568100358423, 0.630272952853, 0.679487179487, 0.759615384613, 0.788888888888, 0.867516923077, 0.933755961538, 0.983865967742, 1.016129032258, 1.066234038461, 1.132478632478, 1.211105000000, 1.240384615382, 1.320506153846, 1.369727047146, 1.431892419355, 1.519230769230, 1.569444444440, 1.648147333330, 1.726850999997, 1.816666666663, 1.870369444443, 1.962961000000, 2.041666666663, 2.138888888883, 2.183333333329, 2.274999999995, 2.366666666666, 2.472222222221, 2.527777777777, 2.633333333332, 2.738888888887, 2.844444444443, 2.949999999999, 3.055555555553, 3.166666666663, 3.277777777775, 3.388888888885, 3.499999999996, 3.611111111108, 3.722222222218, 3.833333333329, 3.944444444441, 4.055555555551, 4.166666666663, 4.277777777773, 4.388888888884, 4.499999999996, 4.611111111106, 4.722222222217, 4.833333333329, 4.944444444439, 5.055555555550, 5.166666666662, 5.277777777772, 5.388888888883};
int val = 7; //this is the amount to move the cursor, this will need to change based on your in game sensitivity.
double delay = 0.0035;   //delay before next SendInput()
double offset = 0.00525; //offset changes the timing to make the first (and all subsequent) sendinputs fire early since there must be a delay between SendInputs or they will get aggregated together
double maxsleepoffset = 0.007; //This program uses a spinlock for maximum precision, but uses sleep in between the spinlock to not eat a full cpu core at all times. This option is the maximum remaining time before switching from sleep based wait to spinlock based wait.
unsigned char onkey = 0x42; //this is the virtual key-code for the hotkey to toggle the program on and off
bool timerres = 1; //whether or not to maximize the timer resolution
bool traymin = 1;  //whether or not to minimize to the system tray
bool nordown = 0;  //whether or not to require right click to be held to activate
INPUT mm[4];

string tempval;
string tempoffset;
string tempdelay;
string tempmaxsleepoffset;
string temponkey;
bool temptimerres = 0;
bool temptraymin = 0;
bool tempnord = 0;
int rdown = 0;
bool on = 1;
bool stopping = 0;

HWND indicatorblue;
HWND indicatorblack;
HBRUSH blue;
HBRUSH black;
WNDPROC fdsa;
string settingsfile;
HMENU traymenu;

HWND mainwin;
HWND helpwin;
HWND helptext;
HWND applybutton;
HWND timerrescheckbox;
HWND traycheckbox;
HWND nordcheckbox;
HWND shakevaledit;
HWND delayedit;
HWND offsetedit;
HWND maxsleepoffsetedit;
HWND onkeyedit;
HWND valtext;
HWND delaytext;
HWND offsettext;
HWND maxsleeptext;
HWND onkeytext;

NOTIFYICONDATA ndata;
HICON thisicon;

void refreshvalues();
void apply();

void load(const char* filename)
{
    uint64_t fsize = 0;
    FILE *fp = fopen(filename, "rb");
    void* strstart;
    if(fp)
    {
        fseek(fp, 0, SEEK_END);
        fsize = (ftell(fp));
        strstart = malloc(fsize);
        rewind(fp);
        fread(strstart, 1, fsize, fp);
        fclose(fp);
    }
    else
    {
        MessageBox(0, "Unable to open file.", "Error!", 0);
        return;
    }
    if(fsize != 34)
    {
        MessageBox(0, "The settings file ARST attempted to load is invalid.", "Settings file invalid!", 0);
        free(strstart);
        return;
    }
    char* cpointer = (char*)strstart;
    if(*cpointer != 0x45 || *(cpointer+1) != 0x73)
    {
        MessageBox(0, "The settings file ARST attempted to load is invalid.", "Settings file invalid!", 0);
        free(strstart);
        return;
    }
    void* arbitrarypointer = strstart;
    arbitrarypointer = (char*)arbitrarypointer+2;
    int* ipointer = (int*)arbitrarypointer;
    val = *ipointer;
    arbitrarypointer = (char*)arbitrarypointer+4;
    double* dpointer = (double*)arbitrarypointer;
    delay = *dpointer;
    offset = *(dpointer+1);
    maxsleepoffset = *(dpointer+2);
    onkey = *(cpointer+30);
    char* bpointer = (char*)strstart+31;
    timerres = (bool*)*bpointer;
    traymin = (bool*)*(bpointer+1);
    nordown = (bool*)*(bpointer+2);
    free(strstart);
    refreshvalues();
    apply();
    return;
}

void save(const char* filename)
{
    char* fnend = (char*)filename;
    while(*fnend!=0)
        fnend++;
    fnend-=4;
    char* asvstart = fnend;
    fnend+=4;
    char* asv = ".asv";
    bool success = 1;
    for(int i = 0;i<4;i++)
    {
        if(*asv != *asvstart)
        {
            success = 0;
            break;
        }
        else
        {
            asv++;
            asvstart++;
        }
    }
    uint64_t ssize = fnend - filename;
    char newfilename[ssize+5];
    if(!success)
    {
        memcpy(newfilename, filename, ssize);
        newfilename[ssize] = '.';
        newfilename[ssize+1] = 'a';
        newfilename[ssize+2] = 's';
        newfilename[ssize+3] = 'v';
        newfilename[ssize+4] = 0;
    }
    char save[34];
    void* vpointer = &save;
    char* cpointer = (char*)vpointer;
    save[0] = 0x45;
    save[1] = 0x73;
    vpointer = (char*)vpointer+2;
    int* ipointer = (int*)vpointer;
    *ipointer = val;
    vpointer = (char*)vpointer+4;
    double* dpointer = (double*)vpointer;
    *dpointer = delay;
    *(dpointer+1) = offset;
    *(dpointer+2) = maxsleepoffset;
    save[30] = onkey;
    vpointer = (char*)(&save)+31;
    cpointer = (char*)vpointer;
    *cpointer = timerres;
    *(cpointer+1) = traymin;
    *(cpointer+2) = nordown;
    FILE *fp;
    if(success)
        fp = fopen(filename, "wb+");
    else
        fp = fopen(newfilename, "wb+");
    if(fp)
    {
        fwrite(save, 1, 34, fp);
        fclose(fp);
    }
    else
    {
        MessageBox(0, "Could not create save file!", "Error!", 0);
    }
    return;
}

bool validatedouble(char *doub, int len)
{
    bool oneperiod = 0;
    for(int i = 0;i<len;i++)
    {
        if(doub[i]=='.')
        {
            if(oneperiod==1)
                return 0;
            else
            {
                oneperiod = 1;
                continue;
            }
        }
        else if(doub[i]>=0x30&&doub[i]<=0x39)
            continue;
        else
            return 0;
    }
    return 1;
}

void apply()
{
    int length = SendMessage(onkeyedit, EM_LINELENGTH, -1, 0);
    if(length==1)
    {
        unsigned char onkeytxt[2] = {0x2, 0x00};
        SendMessage(onkeyedit, EM_GETLINE , 0, (LPARAM)&onkeytxt);
        if(onkeytxt[0]>0x60&&onkeytxt[0]<0x7B)
        {
            onkeytxt[0] = onkeytxt[0]-0x20;
        }
        onkey = onkeytxt[0];
    }
    length = SendMessage(shakevaledit, EM_LINELENGTH, -1, 0);
    if(length<13)
    {
        unsigned char temptxt[13] = {0x0D};
        SendMessage(shakevaledit, EM_GETLINE , 0, (LPARAM)&temptxt);
        for(int i = 0;i<length;i++)
        {
            if(temptxt[i]>=0x30&&temptxt[i]<=0x39)
                continue;
            else
            {
                goto out;
            }
        }
        const char* cp;
        void *required;
        required = &temptxt;
        cp = (const char*)required;
        val = atoi((const char*)temptxt);
    }
    out:
    length = SendMessage(delayedit, EM_LINELENGTH, -1, 0);
    if(length<13)
    {
        unsigned char temptxt[13] = {0x0D};
        SendMessage(delayedit, EM_GETLINE , 0, (LPARAM)&temptxt);
        bool isdouble = validatedouble((char*)temptxt, length);
        if(isdouble)
            delay = atof((const char*)temptxt);
    }
    length = SendMessage(offsetedit, EM_LINELENGTH, -1, 0);
    if(length<13)
    {
        unsigned char temptxt[13] = {0x0D};
        SendMessage(offsetedit, EM_GETLINE , 0, (LPARAM)&temptxt);
        bool isdouble = validatedouble((char*)temptxt, length);
        if(isdouble)
            offset = atof((const char*)temptxt);
    }
    length = SendMessage(maxsleepoffsetedit, EM_LINELENGTH, -1, 0);
    if(length<13)
    {
        unsigned char temptxt[13] = {0x0D};
        SendMessage(maxsleepoffsetedit, EM_GETLINE , 0, (LPARAM)&temptxt);
        bool isdouble = validatedouble((char*)temptxt, length);
        if(isdouble)
            maxsleepoffset = atof((const char*)temptxt);
    }
    uint64_t check = SendMessage(traycheckbox, BM_GETCHECK, 0, 0);
    if(check==BST_CHECKED)
        traymin = 1;
    else
        traymin = 0;
    check = SendMessage(timerrescheckbox, BM_GETCHECK, 0, 0);
    if(check==BST_CHECKED)
    {
        ULONG pointless;
        NtSetTimerResolution(0x1388, 1, &pointless);
        timerres = 1;
    }
    else
    {
        ULONG pointless;
        NtSetTimerResolution(0x138800, 1, &pointless);
        timerres = 0;
    }
    check = SendMessage(nordcheckbox, BM_GETCHECK, 0, 0);
    if(check==BST_CHECKED)
        nordown = 1;
    else
        nordown = 0;
    mm[0].type = 0;
    mm[1].type = 0;
    mm[0].mi.dx = val;
    mm[0].mi.dy = val;
    mm[1].mi.dx = -1*val;
    mm[1].mi.dy = val;
    mm[0].mi.mouseData = 0;
    mm[1].mi.mouseData = 0;
    mm[0].mi.dwFlags = 1;
    mm[1].mi.dwFlags = 1;
    mm[0].mi.time = 0;
    mm[1].mi.time = 0;
    mm[0].mi.dwExtraInfo = 0;
    mm[1].mi.dwExtraInfo = 0;
    mm[2].type = 0;
    mm[3].type = 0;
    mm[2].mi.dx = -1*val;
    mm[2].mi.dy = -1*val;
    mm[3].mi.dx = val;
    mm[3].mi.dy = -1*val;
    mm[2].mi.mouseData = 0;
    mm[3].mi.mouseData = 0;
    mm[2].mi.dwFlags = 1;
    mm[3].mi.dwFlags = 1;
    mm[2].mi.time = 0;
    mm[3].mi.time = 0;
    mm[2].mi.dwExtraInfo = 0;
    mm[3].mi.dwExtraInfo = 0;
    for(int i = 0;i<66;i++)
    {
        timings[i] = timingsoriginal[i]-offset;
    }
    return;
}

void refreshvalues()
{
    string vals = to_string(val);
    string offsets = to_string(offset);
    string delays = to_string(delay);
    string maxsleepoffsets = to_string(maxsleepoffset);
    SetWindowText(shakevaledit, vals.c_str());
    SetWindowText(offsetedit, offsets.c_str());
    SetWindowText(delayedit, delays.c_str());
    SetWindowText(maxsleepoffsetedit, maxsleepoffsets.c_str());
    unsigned char temp[2];
    temp[0] = onkey;
    temp[1] = 0;
    SetWindowText(onkeyedit, (LPCSTR)temp);
    if(timerres)
        SendMessage(timerrescheckbox, BM_SETCHECK, BST_CHECKED, 0);
    else
        SendMessage(timerrescheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
    if(traymin)
        SendMessage(traycheckbox, BM_SETCHECK, BST_CHECKED, 0);
    else
        SendMessage(traycheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
    if(nordown)
        SendMessage(nordcheckbox, BM_SETCHECK, BST_CHECKED, 0);
    else
        SendMessage(nordcheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
    return;
}

struct TIME
{
    uint64_t sec = 0;
    uint64_t nsec = 0;
    TIME operator - (TIME const &t2)
    {
        TIME ret;
        ret.sec = sec - t2.sec;
        if(nsec<t2.nsec)
        {
            ret.sec--;
            ret.nsec+=1000000000;
        }
        ret.nsec = (ret.nsec+nsec)-t2.nsec;
        return ret;
    }
    double to_double()
    {
        return (double)((double)sec+((double)nsec/1000000000));
    }
};

void gettime(TIME &time)
{
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    static uint64_t freq = 0;
    if(!freq)
        QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    QueryPerformanceCounter((LARGE_INTEGER*)&time.sec);
    time.nsec = time.sec%freq;
    time.sec = time.sec/freq;
    time.nsec = time.nsec * 1000000000;
    time.nsec = time.nsec/freq;
    return;
    #else
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    time.sec = ts.tv_sec;
    time.nsec = ts.tv_nsec;
    #endif
    return;
}

TIME time1, time2, time4, time5;
DWORD tickcount;

LRESULT CALLBACK dumbproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    if(umsg==WM_CHAR)
    {
        if(wparam=='.')
        {
            char temps[100];
            int amnt = SendMessage(hwnd, WM_GETTEXT, 100, (LPARAM)temps);
            if(amnt == 0)
                return 0;
            int hasalready = 0;
            for(int i = 0;temps[i];i++)
            {
                if(temps[i]=='.')
                {
                    hasalready = 1;
                }
            }
            if(hasalready)
            {
                return 0;
            }
        }
        else if(!((wparam>='0'&&wparam<='9')||wparam=='.'||wparam==VK_RETURN||wparam==VK_DELETE||wparam==VK_BACK))
        {
            return 0;
        }
    }
    return CallWindowProc(fdsa, hwnd, umsg, wparam, lparam);
}

LRESULT CALLBACK iproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch(message)
    {
        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
    }
    return 0;
}

LRESULT CALLBACK helpproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch(message)
    {
        case WM_CLOSE:
            ShowWindow(helpwin, 0);
            break;
        case WM_DESTROY:
            break;
        case WM_VSCROLL:
            cout << wparam << endl;
            DefWindowProc(helptext, message, wparam, lparam);
            break;
        case WM_LBUTTONDOWN:
            break;
        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
    }
    return 0;
}

LRESULT CALLBACK mainproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    static unsigned char bytes[48];
    static void* svp = &bytes;
    static LPBYTE lpb = (LPBYTE)svp;
    static int size = 48;
    static RAWINPUT* ri = (RAWINPUT*)lpb;
    switch(message)
    {
        case 0x8007:
            switch(LOWORD(lparam))
            {
                case WM_LBUTTONDOWN:
                    ShowWindow(mainwin, 5);
                    Shell_NotifyIcon(2, &ndata);
                    break;
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                    static POINT pt;
                    GetCursorPos(&pt);
                    SetForegroundWindow(hwnd);
                    TrackPopupMenu(traymenu, 0x20, pt.x, pt.y, 0, hwnd, 0);
                    break;
            }
            break;
        case WM_COMMAND:
        {
            if(HIWORD(wparam)==BN_CLICKED)
            {
                if(lparam == (uint64_t)applybutton)
                {
                    apply();
                    refreshvalues();
                    break;
                }
                else if(lparam == (uint64_t)timerrescheckbox)
                {
                    if(temptimerres == 0)
                    {
                        temptimerres = 1;
                        SendMessage(timerrescheckbox, BM_SETCHECK, BST_CHECKED, 0);
                    }
                    else
                    {
                        temptimerres = 0;
                        SendMessage(timerrescheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
                    }
                    break;
                }
                else if(lparam == (uint64_t)traycheckbox)
                {
                    if(temptraymin == 0)
                    {
                        temptraymin = 1;
                        SendMessage(traycheckbox, BM_SETCHECK, BST_CHECKED, 0);
                    }
                    else
                    {
                        temptraymin = 0;
                        SendMessage(traycheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
                    }
                    break;
                }
                else if(lparam == (uint64_t)nordcheckbox)
                {
                    if(tempnord == 0)
                    {
                        tempnord = 1;
                        SendMessage(nordcheckbox, BM_SETCHECK, BST_CHECKED, 0);
                    }
                    else
                    {
                        tempnord = 0;
                        SendMessage(nordcheckbox, BM_SETCHECK, BST_UNCHECKED, 0);
                    }
                    break;
                }
                else
                {
                    switch(LOWORD(wparam))
                    {
                        case 0x8005:
                            ShowWindow(mainwin, 5);
                            Shell_NotifyIcon(2, &ndata);
                            break;
                        case 0x8006:
                            PostQuitMessage(0);
                            break;
                        case 0x9100: //save default
                            PWSTR path;
                            SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, 0, &path);
                            char* cp1;
                            char* cp2;
                            for(int i = 0;true;i++)
                            {
                                cp1=(char*)path+i;
                                cp2=(char*)path+(i*2);
                                *cp1=*cp2;
                                if(*cp2==0)
                                {
                                    break;
                                }
                            }
                            *cp1='\\';
                            *(cp1+1)='d';
                            *(cp1+2)='e';
                            *(cp1+3)='f';
                            *(cp1+4)='a';
                            *(cp1+5)='u';
                            *(cp1+6)='l';
                            *(cp1+7)='t';
                            *(cp1+8)='.';
                            *(cp1+9)='a';
                            *(cp1+10)='s';
                            *(cp1+11)='v';
                            *(cp1+12)=0;
                            save((const char*)path);
                            break;
                        case 0x9101: //load
                        {
                            OPENFILENAME ofn;
                            ZeroMemory(&ofn, sizeof(ofn));
                            char szFile[1000];
                            ofn.lStructSize = sizeof(ofn);
                            ofn.hwndOwner = 0;
                            ofn.lpstrFile = szFile;
                            ofn.lpstrFile[0] = 0;
                            ofn.nMaxFile = sizeof(szFile);
                            ofn.lpstrFilter = "ARST binary save file\0*.asv\0";
                            ofn.nFilterIndex = 1;
                            ofn.lpstrFileTitle = 0;
                            ofn.nMaxFileTitle = 0;
                            ofn.lpstrInitialDir = 0;
                            ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
                            bool success = GetOpenFileName(&ofn);
                            if(success)
                            {
                                load(ofn.lpstrFile);
                            }
                            else if(CommDlgExtendedError())
                            {
                                MessageBox(0, "Unable to open the file.", "Error!", 0);
                            }
                            break;
                        }
                        case 0x9102: //saveas
                        {
                            OPENFILENAME ofn;
                            ZeroMemory(&ofn, sizeof(ofn));
                            char szFile[1000];
                            ofn.lStructSize = sizeof(ofn);
                            ofn.hwndOwner = 0;
                            ofn.lpstrFile = szFile;
                            ofn.lpstrFile[0] = 0;
                            ofn.nMaxFile = sizeof(szFile);
                            ofn.lpstrFilter = "ARST binary save file\0*.asv\0";
                            ofn.nFilterIndex = 1;
                            ofn.lpstrFileTitle = 0;
                            ofn.nMaxFileTitle = 0;
                            ofn.lpstrInitialDir = 0;
                            ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
                            bool success = GetSaveFileName(&ofn);
                            if(success)
                            {
                                save(ofn.lpstrFile);
                                break;
                            }
                            else if(CommDlgExtendedError())
                            {
                                MessageBox(0, "Unable to create save file.", "Error!", 0);
                            }
                            break;
                        }
                        case 0x9103: //help
                            ShowWindow(helpwin, 5);
                            break;
                        case 0x9104: //revert defaults
                            val = 7;
                            delay = 0.0035;
                            offset = 0.00525;
                            maxsleepoffset = 0.007;
                            onkey = 0x42;
                            timerres = 0;
                            traymin = 0;
                            nordown = 0;
                            refreshvalues();
                            apply();
                            break;
                        case 0x9105: //exit
                            PostQuitMessage(0);
                            break;
                    }
                }
                break;
            }
            break;
        }
        case WM_INPUT:
        {
            GetRawInputData((HRAWINPUT)lparam, RID_INPUT, lpb, (PUINT)&size, sizeof(RAWINPUTHEADER));
            if(ri->header.dwType == RIM_TYPEMOUSE && ri->data.mouse.usButtonFlags&1)
            {
                stopping = 0;
            }
            if(ri->header.dwType == RIM_TYPEKEYBOARD)
            {
                if(ri->data.keyboard.VKey == onkey && ri->data.keyboard.Message == WM_KEYDOWN)
                {
                    if(on == 1)
                    {
                        on = 0;
                        ShowWindow(indicatorblue, 0);
                        ShowWindow(indicatorblack, 5);
                    }
                    else if(on == 0)
                    {
                        on = 1;
                        ShowWindow(indicatorblue, 5);
                        ShowWindow(indicatorblack, 0);
                    }
                }
            }
            break;
        }
        case WM_CLOSE:
            if(traymin)
            {
                ShowWindow(mainwin, 0);
                Shell_NotifyIcon(0, &ndata);
            }
            else
                PostQuitMessage(0);
            break;
        case WM_DESTROY:
            break;
        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
    }
    return 0;
}

void mainspin()
{
    mm[0].type = 0;
    mm[1].type = 0;
    mm[0].mi.dx = val;
    mm[0].mi.dy = val;
    mm[1].mi.dx = -1*val;
    mm[1].mi.dy = val;
    mm[0].mi.mouseData = 0;
    mm[1].mi.mouseData = 0;
    mm[0].mi.dwFlags = 1;
    mm[1].mi.dwFlags = 1;
    mm[0].mi.time = 0;
    mm[1].mi.time = 0;
    mm[0].mi.dwExtraInfo = 0;
    mm[1].mi.dwExtraInfo = 0;
    mm[2].type = 0;
    mm[3].type = 0;
    mm[2].mi.dx = -1*val;
    mm[2].mi.dy = -1*val;
    mm[3].mi.dx = val;
    mm[3].mi.dy = -1*val;
    mm[2].mi.mouseData = 0;
    mm[3].mi.mouseData = 0;
    mm[2].mi.dwFlags = 1;
    mm[3].mi.dwFlags = 1;
    mm[2].mi.time = 0;
    mm[3].mi.time = 0;
    mm[2].mi.dwExtraInfo = 0;
    mm[3].mi.dwExtraInfo = 0;
    unsigned short ldown = 0;
    while(true)
    {
        Sleep(1);
        int time = 0;
        int once = 1;
        ldown = GetAsyncKeyState(1);
        ldown = ldown&32768;
        if(ldown != 32768)
            gettime(time1);
        rdown = GetAsyncKeyState(2);
        rdown = rdown&32768;
        if(nordown)
            rdown = 32768;
        while(ldown == 32768 && rdown == 32768 && on == 1 && stopping == 0)
        {
            gettime(time2);
            TIME time3 = time2-time1;
            if(once)
            {
                once = 0;
                double check = time3.to_double();
                for(int i = 0;i<65;i++)
                {
                    if(check<=timings[i])
                    {
                        time = i;
                        break;
                    }
                }
            }
            if(time3.to_double()>=timings[time])
            {
                time++;
                SendInput(1,mm,sizeof(INPUT));
                gettime(time4);
                while(true)
                {
                    gettime(time5);
                    TIME time6 = time5-time4;
                    if(time6.to_double()>=delay)
                    {
                        SendInput(1,&mm[1],sizeof(INPUT));
                        break;
                    }
                }
                gettime(time4);
                while(true)
                {
                    gettime(time5);
                    TIME time6 = time5-time4;
                    if(time6.to_double()>=delay)
                    {
                        SendInput(1,&mm[2],sizeof(INPUT));
                        break;
                    }
                }
                gettime(time4);
                while(true)
                {
                    gettime(time5);
                    TIME time6 = time5-time4;
                    if(time6.to_double()>=delay)
                    {
                        SendInput(1,&mm[3],sizeof(INPUT));
                        break;
                    }
                }
                ldown = GetAsyncKeyState(1);
                ldown = ldown&32768;
                rdown = GetAsyncKeyState(2);
                rdown = rdown&32768;
                if(nordown)
                    rdown = 32768;
                if(time>65)
                {
                    stopping = 1;
                    break;
                }
            }
            else if(time3.to_double()<=timings[time]-maxsleepoffset)
                Sleep(1);
        }
    }
    return;
}

int WINAPI WinMain(HINSTANCE hthisinst, HINSTANCE hprevinst, LPSTR lpszArgument, int nCmdShow)
{
    NtSetTimerResolution = (void (*)(ULONG, bool, PULONG))GetProcAddress(GetModuleHandle("ntdll.dll"), "NtSetTimerResolution");
    traymenu = CreatePopupMenu();
    InsertMenu(traymenu, -1, 0x400, 0x8005, "Settings");
    InsertMenu(traymenu, -1, 0x400, 0x8006, "Exit");
    thisicon = LoadIcon(hthisinst, "MAINICON");
    ZeroMemory(&ndata, sizeof(ndata));
    ndata.cbSize = sizeof(ndata);
    ndata.uID = 17;
    ndata.uFlags = 3;
    ndata.uCallbackMessage = 0x8007;
    ndata.hIcon = thisicon;
    if(offset)
    {
        for(int i = 0;i<66;i++)
        {
            timings[i] = timings[i] - offset;
        }
    }
    blue = CreateSolidBrush(RGB(0,0,255));
    black = CreateSolidBrush(RGB(0,0,0));
    MSG msg;
    WNDCLASSEX wc, ic, ic2, hc;
    wc.hInstance = hthisinst;
    wc.lpszClassName = "mainclass";
    wc.lpfnWndProc = mainproc;
    wc.style = 0;
    wc.hIcon = thisicon;
    wc.hIconSm = LoadIcon(NULL, 0);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    hc.hInstance = hthisinst;
    hc.lpszClassName = "helpclass";
    hc.lpfnWndProc = helpproc;
    hc.style = 0;
    hc.hIcon = thisicon;
    hc.hIconSm = LoadIcon(NULL, 0);
    hc.hCursor = LoadCursor(NULL, IDC_ARROW);
    hc.cbSize = sizeof(WNDCLASSEX);
    hc.lpszMenuName = 0;
    hc.cbClsExtra = 0;
    hc.cbWndExtra = 0;
    hc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
    ic.hInstance = hthisinst;
    ic.lpszClassName = "indicatorblue";
    ic.lpfnWndProc = iproc;
    ic.style = 0;
    ic.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    ic.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    ic.hCursor = LoadCursor(NULL, IDC_ARROW);
    ic.cbSize = sizeof(WNDCLASSEX);
    ic.lpszMenuName = NULL;
    ic.cbClsExtra = 0;
    ic.cbWndExtra = 0;
    ic.hbrBackground = blue;
    ic2.hInstance = hthisinst;
    ic2.lpszClassName = "indicatorblack";
    ic2.lpfnWndProc = iproc;
    ic2.style = 0;
    ic2.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    ic2.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    ic2.hCursor = LoadCursor(NULL, IDC_ARROW);
    ic2.cbSize = sizeof(WNDCLASSEX);
    ic2.lpszMenuName = NULL;
    ic2.cbClsExtra = 0;
    ic2.cbWndExtra = 0;
    ic2.hbrBackground = black;
    WNDCLASSEX asdfz;
    asdfz.cbSize = sizeof(WNDCLASSEX);
    GetClassInfoEx(0, "edit", &asdfz);
    fdsa = asdfz.lpfnWndProc;
    asdfz.lpfnWndProc = dumbproc;
    asdfz.lpszClassName = "floatedit";
    asdfz.hInstance = hthisinst;
    if(!RegisterClassEx(&wc))
        return 0;
    if(!RegisterClassEx(&ic))
        return 0;
    if(!RegisterClassEx(&ic2))
        return 0;
    if(!RegisterClassEx(&asdfz))
    {
        return 0;
    }
    if(!RegisterClassEx(&hc))
    {
        return 0;
    }
    mainwin = CreateWindowEx(0, "mainclass", "Automatic recoil smoothing tool (ARST)", WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX, 0x80000000, 0x80000000, 315, 255, HWND_DESKTOP, NULL, hthisinst, NULL);
    helpwin = CreateWindowEx(0, "helpclass", "Help", WS_OVERLAPPEDWINDOW, 0x80000000, 0x80000000, 1000, 750, HWND_DESKTOP, NULL, hthisinst, NULL);
    indicatorblue = CreateWindowEx(0, "indicatorblue", "indicatorblue", WS_POPUP, 0, 1075, 5, 5, mainwin, NULL, hthisinst, NULL);
    indicatorblack = CreateWindowEx(0, "indicatorblack", "indicatorblack", WS_POPUP, 0, 1075, 5, 5, mainwin, NULL, hthisinst, NULL);
    applybutton = CreateWindowEx(0, "button", "Apply", WS_GROUP|WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP, 250, 155, 50, 30, mainwin, NULL, hthisinst, NULL);
    timerrescheckbox = CreateWindowEx(0, "button", "Maximize timer resolution", WS_GROUP|WS_VISIBLE|WS_CHILD|BS_CHECKBOX|WS_TABSTOP, 10, 135, 190, 20, mainwin, 0, hthisinst, 0);
    traycheckbox = CreateWindowEx(0, "button", "Minimize to tray on close", WS_GROUP|WS_VISIBLE|WS_CHILD|BS_CHECKBOX|WS_TABSTOP, 10, 160, 185, 20, mainwin, 0, hthisinst, 0);
    nordcheckbox = CreateWindowEx(0, "button", "Don't require right-click", WS_GROUP|WS_VISIBLE|WS_CHILD|BS_CHECKBOX|WS_TABSTOP, 10, 185, 205, 20, mainwin, 0, hthisinst, 0);
    shakevaledit = CreateWindowEx(0, "edit", 0, WS_GROUP|WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_NUMBER, 10, 10, 80, 20, mainwin, NULL, hthisinst, NULL);
    delayedit = CreateWindowEx(0, "floatedit", 0, WS_GROUP|WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP, 10, 35, 80, 20, mainwin, NULL, hthisinst, NULL);
    offsetedit = CreateWindowEx(0, "floatedit", 0, WS_GROUP|WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP, 10, 60, 80, 20, mainwin, NULL, hthisinst, NULL);
    maxsleepoffsetedit = CreateWindowEx(0, "floatedit", 0, WS_GROUP|WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP, 10, 85, 80, 20, mainwin, NULL, hthisinst, NULL);
    onkeyedit = CreateWindowEx(0, "edit", 0, WS_GROUP|WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP, 10, 110, 80, 20, mainwin, NULL, hthisinst, NULL);
    valtext = CreateWindowEx(0, "static", "Cursor movement amount", WS_CHILD|WS_VISIBLE, 100, 10, 200, 20, mainwin, NULL, hthisinst, NULL);
    delaytext = CreateWindowEx(0, "static", "Delay between movements", WS_CHILD|WS_VISIBLE, 100, 35, 200, 20, mainwin, NULL, hthisinst, NULL);
    offsettext = CreateWindowEx(0, "static", "Negative timing offset", WS_CHILD|WS_VISIBLE, 100, 60, 200, 20, mainwin, NULL, hthisinst, NULL);
    maxsleeptext = CreateWindowEx(0, "static", "Precision/CPU usage tradeoff", WS_CHILD|WS_VISIBLE, 100, 85, 200, 20, mainwin, NULL, hthisinst, NULL);
    onkeytext = CreateWindowEx(0, "static", "ARST disable/enable hotkey", WS_CHILD|WS_VISIBLE, 100, 110, 200, 20, mainwin, NULL, hthisinst, NULL);
    HMENU mainmenu = CreateMenu();
    HMENU submenu = CreatePopupMenu();
    SendMessage(onkeyedit, EM_SETLIMITTEXT, 1, 0);
    AppendMenu(submenu, MF_STRING, 0x9101, "&Load");
    AppendMenu(submenu, MF_STRING, 0x9100, "&Save as default");
    AppendMenu(submenu, MF_STRING, 0x9102, "&Save as...");
    AppendMenu(submenu, MF_STRING, 0x9103, "&Help");
    AppendMenu(submenu, MF_STRING, 0x9104, "&Revert defaults");
    AppendMenu(submenu, MF_STRING, 0x9105, "&Exit");
    AppendMenu(mainmenu, MF_STRING|MF_POPUP, (uint64_t)submenu, "&Menu");
    SetMenu(mainwin, mainmenu);
    ShowWindow(mainwin, 5);
    ShowWindow(indicatorblue, 5);
    SetWindowPos(indicatorblue, (HWND)-1, 0, 1075, 5, 5, 0);
    SetWindowPos(indicatorblack, (HWND)-1, 0, 1075, 5, 5, 0);
    RAWINPUTDEVICE Rid[2];
    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x02;
    Rid[0].dwFlags = 0x100;
    Rid[0].hwndTarget = mainwin;
    Rid[1].usUsagePage = 0x01;
    Rid[1].usUsage = 0x06;
    Rid[1].dwFlags = 0x100;
    Rid[1].hwndTarget = mainwin;
    if(RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE)
    {
        cout << "rawinput registration failed.\n";
        return 3;
    }
    HANDLE hthread;
    PWSTR path;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, 0, &path);
    char* cp1;
    char* cp2;
    for(int i = 0;true;i++)
    {
        cp1=(char*)path+i;
        cp2=(char*)path+(i*2);
        *cp1=*cp2;
        if(*cp2==0)
        {
            break;
        }
    }
    *cp1=0x5C; //5c is backslash
    *(cp1+1)='d';
    *(cp1+2)='e';
    *(cp1+3)='f';
    *(cp1+4)='a';
    *(cp1+5)='u';
    *(cp1+6)='l';
    *(cp1+7)='t';
    *(cp1+8)='.';
    *(cp1+9)='a';
    *(cp1+10)='s';
    *(cp1+11)='v';
    *(cp1+12)=0;
    FILE *check;
    if((check = fopen((const char*)path, "r")))
    {
        fclose(check);
        load((const char*)path);
    }
    hthread = CreateThread(0,0, (LPTHREAD_START_ROUTINE)&mainspin, 0, 0, 0);
    refreshvalues();
    ndata.hWnd = mainwin;
    helptext = CreateWindowEx(0, "static", "ARST (automatic recoil smoothing tool) is a program to trigger recoil smoothing while shooting in apex legends with as little side effects as possible. Recoil smoothing is a game mechanic where if you are aiming down sights and your FOV is lower than your normal looking FOV (that is, you're zoomed in) the effect of recoil is reduced when you're moving your cursor. This tool works by making small diamond shaped movements with the cursor, just enough to trigger recoil smoothing while shooting without moving your cursor.\n\
\n\
Cursor movement amount: \n\
This is the amount of pixels that ARST will move your cursor per input. You want this value to be 20 divided by your sensitivity rounded up, for example for a sensitivity of 5 you would set this value to 4, for a sensitivity of 3 you would set this to 7. Too high and you may unnecessarily see stronger screen shake, too low and you won't get the maximum effect of recoil smoothing.\n\
\n\
Delay between movements: \n\
This determines how long the program will wait after sending a cursor movement input before sending another. This delay is required because if input is sent to the system too close together for some reason apex aggregates this data together and treats it as one movement, so for example if you do a full circle and then stop your cursor where it started apex will treat it as though you didn't move your cursor at all and you will get no recoil smoothing. 0.0035 seems to be the lowest setting on my system that I can set this to and still not have apex aggregate my inputs together. Because this might vary from system to system, I suggest you adjust this setting after you've made sure cursor movement amount is set correctly if you're not getting the effect of recoil smoothing in game and you're getting cursor movements while testing out of game.\n\
\n\
Negative timing offset:\n\
This value determines how much time should be left until it hits one of it's internal timing intervals before it sends input to move the cursor in a circle. This value should almost certainly be 1.5x the value of \"delay input movements\".\n\
\n\
Precision/CPU usage tradeoff:\n\
In order to guarentee the degree of accuracy this program needs to work correctly, the program uses whats called a \"spin-lock\", in laymans terms it means this program attempts to eat up a full cpu core until it's ready to send inputs. To mitigate this, it uses a different technique (Sleep) to wait an inprecise amount of time until a certain amount of time is left and then switches to the spinlock method.\n\
The Precision/CPU usage tradeoff value controls the maximum remaining time before switching from sleep based wait to spinlock based wait. A higher value means less CPU usage but lower precision input timing. On my system(intel 8700k cpu) I ran some tests and found the maximum sleep overshoot with apex running almost never exceeded 0.007 seconds, so I've set that as the default value.\n\
\n\
Maximize timer resolution:\n\
The default timer resolution on most systems is 16ms (0.016 seconds) but usually can go as low as 0.5ms. Changing this value will most likely help to keep precise sleep times, so I have enabled it by default. Only change this setting if you know what you're doing.\n\
\n\
Minimize to tray on close:\n\
This convenience feature will allow you to close the program and have it minimize to the tray so you don't have to deal with it being on your taskbar.\n\
\n\
Don't require right-click:\n\
Some people don't use hold right click to aim down sights, if you don't use hold right click to aim down sights turn this setting off and the program will always activate on left click rather than requiring right click and left click to be held.", WS_CHILD|WS_VISIBLE, 10, 10, 950, 690, helpwin, NULL, hthisinst, NULL);

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
