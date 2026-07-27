module slash_i;
import visual;
import itunes_c;
import itunes_x;
import <Windows.h>;
import <shobjidl.h>;
import <ShlObj.h>;

void iTunes::remove_track() {
    DISPID dispidDelete;
    const OLECHAR* szDelete = L"Delete";
    BSTR bstrDelete = SysAllocString(szDelete);
    hr = p_current_track->GetIDsOfNames(IID_NULL, &bstrDelete, 1, LOCALE_USER_DEFAULT, &dispidDelete);
    SysFreeString(bstrDelete);
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    hr = p_current_track->Invoke(dispidDelete, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, NULL, NULL, NULL);
    p_current_track = nullptr;
    recycle_bin_track();
}

void iTunes::recycle_bin_track() {
    SHFILEOPSTRUCT shFileOp = {0};
    shFileOp.wFunc = FO_DELETE;
    std::wstring doubleNullTermPath = track_location + L'\0';
    shFileOp.pFrom = doubleNullTermPath.c_str();
    shFileOp.fFlags = FOF_ALLOWUNDO | FOF_NO_UI;
    std::wstring logg_message;
    if (SHFileOperation(&shFileOp) != 0) {
        logg_message = L"Error - file not moved to the recycle bin: " + track_location;
    }
    else {
        logg_message = L"File moved to the recycle bin: " + track_location;
    }
    iTunes_logger.logg_and_logg(logg_message);
}

void iTunes::delete_track() {
    std::wstring logg_message;
    if (!DeleteFile(track_location.c_str())) {
        logg_message = L"Error - file not deleted: " + track_location;
    }
    else {
        logg_message = L"File deleted: " + track_location;
    }
    iTunes_logger.logg_and_logg(logg_message);
}

void remove_iTunes_song() {
    iTunes_logger.logg_and_logg("remove_iTunes_song()");
    ac_iTunes.get_current_track();
    if (ac_iTunes.p_current_track) {
        ac_iTunes.remove_track();
    }
}
