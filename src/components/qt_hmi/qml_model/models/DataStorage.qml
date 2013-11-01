/**
 * @file DataStorage.qml
 * @brief Container for data.
 * Copyright (c) 2013, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

import QtQuick 2.0
import "../hmi_api/Common.js" as Common
import "Internal.js" as Internal
import "Constants.js" as Constants

QtObject {

    property string contactsFirstLetter // first letter of contact's name that need to find at contact list
    property ApplicationModel currentApplication: ApplicationModel { }
    property SliderModel uiSlider: SliderModel { }
    property PerformAudioPassThruModel uiAudioPassThru: PerformAudioPassThruModel { }

    function getApplication(appId) {
        console.log("dataContainer getApplication enter");
        for(var i = 0; i < applicationList.count; i++) {
            if(applicationList.get(i).appId === appId) {
                console.log("Application found", applicationList.get(i));
                return applicationList.get(i);
            }
        }
        console.log("dataContainer getApplication exit");
    }

    onApplicationListChanged: {
        console.log("onApplicationListChanged()");
        setCurrentApplication(appId);
    }

    function setCurrentApplication(appId) {
        console.log("Enter setCurrentApplication function");
        var oldApplicationContext = applicationContext;
        if (currentApplication.appId !== appId) {
            applicationContext = false;
        }

        for(var i = 0; i < applicationList.count; i++) {
            if(applicationList.get(i).appId === appId) {
                currentApplication.appId = appId
                var application = applicationList.get(i)
                currentApplication.appName = application.appName
                currentApplication.appType = application.appType
                currentApplication.playPauseState = application.playPauseState
                currentApplication.options = application.options

                if (application.hmiUIText.mainField1) {
                    currentApplication.hmiUIText.mainField1 = application.hmiUIText.mainField1
                }
                if (application.hmiUIText.mainField2) {
                    currentApplication.hmiUIText.mainField2 = application.hmiUIText.mainField2
                }
                if (application.hmiUIText.mainField3) {
                    currentApplication.hmiUIText.mainField3 = application.hmiUIText.mainField3
                }
                if (application.hmiUIText.mainField4) {
                    currentApplication.hmiUIText.mainField4 = application.hmiUIText.mainField4
                }
                if (application.hmiUIText.statusBar) {
                    currentApplication.hmiUIText.statusBar = application.hmiUIText.statusBar
                }
                if (application.hmiUIText.picture) {
                    currentApplication.hmiUIText.picture = application.hmiUIText.picture
                }
                currentApplication.deviceName = application.deviceName
                currentApplication.isMediaApplication = application.isMediaApplication
                currentApplication.turnList = application.turnList
                currentApplication.turnListSoftButtons = application.turnListSoftButtons
                currentApplication.mediaClock.restore(application.mediaClock.updateMode, application.mediaClock.runningMode, application.mediaClock.magic, application.mediaClock.total)
                currentApplication.languageTTSVR = application.languageTTSVR
                currentApplication.hmiDisplayLanguageDesired = application.hmiDisplayLanguageDesired
                // This place is for adding new properties
            }
        }
        applicationContext = oldApplicationContext;
        currentApplicationChanged()
        console.log("Exit setCurrentApplication function")
    }

    function addApplication(app) {
        console.log("Enter addApplication function");
        applicationList.append({
            appName: app.appName,
            ngnMediaScreenAppName: app.ngnMediaScreenAppName,
            icon: app.icon,
            deviceName: app.deviceName,
            appId: app.appId,
            hmiDisplayLanguageDesired: app.hmiDisplayLanguageDesired,
            isMediaApplication: app.isMediaApplication,
            appType: app.appType,
            helpPrompt: "",
            timeoutPrompt: "",
            playPauseState: 'Pause',
            hmiUIText: app.hmiUIText,
            options: [],
            turnList: [],
            turnListSoftButtons: [],
            mediaClock: app.mediaClock,
            languageTTSVR: Common.Language.EN_US
            // This place is for adding new properties
        })
        console.log("Exit addApplication function");
    }

    function setApplicationProperties(appId, props) {
        console.log("Enter setApplicationProperties function");
        var app = getApplication(appId)
        for (var p in props) {
            app[p] = props[p];
        }
        if (currentApplication.appId === appId) {
            setCurrentApplication(appId); // copy new values to current application
        }
        console.log("Exit setApplicationProperties function");
    }

    function removeApplication(appId) {
        console.log("Enter removeApplication function");
        for (var i = 0; i < applicationList.count; i++) {
            if (applicationList.get(i).appId === appId) {
                applicationList.remove(i);
                break;
            }
        }
        console.log("Exit removeApplication function");
    }
    property int systemContext

    property int hmiContext
    property bool applicationContext: false

    property int systemSavedContext
    property bool applicationSavedContext

    property string route_text: ""

    property PlayerState cdPlayerState: PlayerState {
        playPauseState: 'Pause'
        albumImage: "../res/album_art.png"
        trackNumber: "13/16"
        trackName: "The Dog Days Are Over"
        albumName: "Florence and the Machine"
    }
    property PlayerState ipodPlayerState: PlayerState {
        albumImage: "../res/album_art.png"
        trackNumber: "13/16"
        trackName: "The Dog Days Are Over"
        albumName: "Florence and the Machine"
        playPauseState: 'Pause'
    }
    property PlayerState usbPlayerState: PlayerState {
        playPauseState: 'Pause'
        albumImage: "../res/album_art.png"
        trackNumber: "13/16"
        trackName: "The Dog Days Are Over"
        albumName: "Florence and the Machine"
    }
    property PlayerState linePlayerState: PlayerState {
        playPauseState: 'Pause'
        albumImage: "../res/album_art.png"
        trackNumber: "13/16"
        trackName: "The Dog Days Are Over"
        albumName: "Florence and the Machine"
    }
    property PlayerState btPlayerState: PlayerState {
        playPauseState: 'Pause'
        albumImage: "../res/album_art.png"
        trackNumber: "13/16"
        trackName: "The Dog Days Are Over"
        albumName: "Florence and the Machine"
    }
    property PlayerState amPlayerState: PlayerState {
        songName: "So Far Around The Bend"
        albumName: "The National"
        presets: [ "1130", "950", "760", "1270", "1400", "2100", "940", "1001" ]
    }
    property PlayerState fmPlayerState: PlayerState {
        songName: "So Far Around The Bend"
        albumName: "The National"
        presets: ["96.3", "107.9", "104.3", "101.9", "105.3", "100.5", "107.9", "103.4"]
    }
    property PlayerState siriusPlayerState: PlayerState {
        songName: "So Far Around The Bend"
        albumName: "The National"
        presets: [ "Lithium", "Spectrum", "ESPN", "Alt Nation", "Lithium", "Spectrum", "ESPN", "Alt Nation" ]
    }

    property bool hmiVRAvailable: false
    property bool hmiTTSAvailable: false
    property bool hmiNavigationAvailable: false
    property bool hmiVehicleInfoAvailable: false
    property bool hmiUIAvailable: false

    property int hmiUILanguage: Common.Language.EN_US
    property int hmiTTSVRLanguage: Common.Language.EN_US

    property int hmiUITextAlignment: Text.AlignLeft

    property ListModel deviceList: ListModel { }
    property ListModel applicationList: ListModel { }

    property ListModel vrCommands: ListModel {}

    property ListModel vrHelp: ListModel {}

    property var globalProperties: {
        "helpPrompt": "",
        "timeoutPrompt": ""
    }

    function reset () {
        console.log("dataContainer reset enter");
        route_text = ""
        console.log("dataContainer reset exit");
    }

    function changeRegistrationUI (language, appID) {
        console.log("dataContainer changeRegistrarionUI enter");
        setApplicationProperties(appID, { hmiDisplayLanguageDesired: language });
        console.log("dataContainer changeRegistrarionUI exit");
    }

    function changeRegistrationTTSVR(language, appID) {
        console.log("dataContainer changeRegistrationTTSVR enter");
        setApplicationProperties(appID, { TTSVRLanguage: language });
        console.log("dataContainer changeRegistrationTTSVR exit");
    }

    function addCommand (cmdID, menuParams, cmdIcon, appID) {
        console.debug("enter: " +
                      cmdID +
                      ", " +
                      (menuParams ?
                          "{" + menuParams.parentID + ", " + menuParams.position + ", " + menuParams.menuName + "}" : menuParams) +
                          ", " +
                      (cmdIcon ?
                          "{" + cmdIcon.value + ", " + cmdIcon.imageType + "}" : cmdIcon) +
                      ", " +
                      appID
        )
        if ((menuParams !== undefined) && (menuParams.parentID !== undefined)) {
            var parentNotFound = true
            for (var optionIndex = 0; optionIndex < getApplication(appID).options.count; ++optionIndex) {
                var option = getApplication(appID).options.get(optionIndex)
                if ((option.type === Internal.MenuItemType.MI_SUBMENU) && (option.id === menuParams.parentID)) {
                    parentNotFound = false
                    var count = option.subMenu.count - 1 // decremented because of "back" item
                    if (count < Constants.maximumCommandsPerSubmenu) {
                        var index
                        if ((menuParams.position !== undefined) && (menuParams.position < count)) {
                            index = menuParams.position
                        }
                        else {
                            index = count
                        }
                        ++index // incremented because of "back" item with index 0
                        option.subMenu.insert(index, {"id": cmdID, "name": menuParams.menuName, "type": Internal.MenuItemType.MI_NODE, "icon": cmdIcon ? cmdIcon : {}, "subMenu": []})
                    }
                    else {
                        console.log("addCommand(): too many commands in submenu id = " + option.id + ", rejecting")
                        throw Common.Result.REJECTED
                    }
                    break
                }
            }
            if (parentNotFound) {
                console.log("UI::addCommand(): parentID " + menuParams.parentID + " not found")
            }
        }
        else {
            count = getApplication(appID).options.count
            if (count < Constants.maximumCommandsPerSubmenu) {
                if ((menuParams.position !== undefined) && (menuParams.position < count)) {
                    index = menuParams.position
                }
                else {
                    index = count
                }
                var name = menuParams ? menuParams.menuName : "cmdID = " + cmdID
                getApplication(appID).options.insert(index, {"id": cmdID, "name": name, "type": Internal.MenuItemType.MI_NODE, "icon": cmdIcon ? cmdIcon : {}, "subMenu": []})
            }
            else {
                console.log("addCommand(): too many commands in root menu, rejecting")
                throw Common.Result.REJECTED
            }
        }
        console.debug("exit")
    }

    function deleteCommand (cmdID, appID) {
        console.debug("enter: " + cmdID + ", " + appID)
        for (var optionIndex = 0; optionIndex < getApplication(appID).options.count; ++optionIndex) {
            var option = getApplication(appID).options.get(optionIndex)
            if (option.type === Internal.MenuItemType.MI_NODE) {
                if (option.id === cmdID) {
                    getApplication(appID).options.remove(optionIndex)
                    break
                }
            }
            if (option.type === Internal.MenuItemType.MI_SUBMENU) {
                var subMenu = option.subMenu
                var idMatchFound = false
                for (var subOptionIndex = 0; subOptionIndex < subMenu.count; ++subOptionIndex) {
                    if (subMenu.get(subOptionIndex).id === cmdID) {
                        idMatchFound = true
                        if (subMenu !== currentApplication.currentSubMenu) {
                            subMenu.remove(subOptionIndex)
                        }
                        else {
                            console.log("UI::deleteCommand(): cannot remove item from current submenu, rejecting")
                            throw Common.Result.REJECTED
                        }
                        break
                    }
                }
                if (idMatchFound) {
                    break
                }
            }
        }
        console.debug("exit")
    }

    function addSubMenu (menuID, menuParams, appID) {
        console.debug("enter: " + menuID + ", {" + menuParams.parentID + ", " + menuParams.position + ", " + menuParams.menuName + "}, " + appID)
        var count = getApplication(appID).options.count
        if (count < Constants.maximumSubmenus) {
            var index
            if ((menuParams.position !== undefined) && (menuParams.position < count)) {
                index = menuParams.position
            }
            else {
                index = count
            }
            getApplication(appID).options.insert(index, {
                "id": menuID,
                "name": menuParams.menuName,
                "type": Internal.MenuItemType.MI_SUBMENU,
                "icon": undefined,
                "subMenu": [{
                    "id": -1,
                    "name": menuParams.menuName,
                    "type": Internal.MenuItemType.MI_PARENT,
                    "icon": {
                        "imageType": Common.ImageType.DYNAMIC,
                        "value": "../res/nav/turnArrow.png"
                    },
                    "subMenu": getApplication(appID).options
                }]
            })
        }
        else {
            console.log("addSubMenu(): too many submenus, rejecting")
            throw Common.Result.REJECTED
        }
        console.debug("exit")
    }

    function deleteSubMenu (menuID, appID) {
        console.debug("enter: " + menuID + ", " + appID)
        for (var optionIndex = 0; optionIndex < getApplication(appID).options.count; ++optionIndex) {
            var option = getApplication(appID).options.get(optionIndex)
            if ((option.type === Internal.MenuItemType.MI_SUBMENU) && (option.id === menuID)) {
                if (option.subMenu !== currentApplication.currentSubMenu) {
                    getApplication(appID).options.remove(optionIndex)
                }
                else {
                    console.log("UI::deleteSubMenu(): cannot remove current submenu, rejecting")
                    throw Common.Result.REJECTED
                }
                break
            }
        }
        console.debug("exit")
    }

    function setVrHelp (vrHelp) {
        this.vrHelp.clear()
        var index
        for (var i = 0; i < vrHelp.length; ++i) {
            index = 0
// sort by simple inserts
            while ((index < this.vrHelp.count) && (this.vrHelp.get(index).position < vrHelp[i].position)) {
                ++index
            }
            this.vrHelp.insert(index, {
                                            "text": vrHelp[i].text,
                                            "icon": vrHelp[i].image ? vrHelp[i].image : {},
                                            "position": vrHelp[i].position
                                        })
        }
    }

    property NavigationModel navigationModel: NavigationModel { }
    property VehicleInfoModel vehicleInfoModel: VehicleInfoModel { }
    property ScrollableMessageModel scrollableMessageModel: ScrollableMessageModel { }
    property bool activeVR: false

    property InteractionModel interactionModel: InteractionModel {
    }

    property int driverDistractionState: Common.DriverDistractionState.DD_OFF
    onDriverDistractionStateChanged: {
        sdlUI.onDriverDistraction(driverDistractionState);
    }
    property bool activeTTS: false
    property var activePopup
}
