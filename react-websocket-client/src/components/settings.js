import React, { useState, useEffect, useRef } from 'react';
import 'react-bootstrap-range-slider/dist/react-bootstrap-range-slider.css';
import SettingsCheckBox from './settingscheckbox';
import SettingsSlider from './settingsSlider';
import Button from 'react-bootstrap/Button';

function Settings({startupData}) {
    
      function handleSaver() {
        
        // Send data to the backend via POST
        fetch('http://192.168.2.1:80/control?var=savesetting&val=1', {  // Enter your IP address here
    
          method: 'GET', 
          mode: 'cors'
    
        })
        
      }

      function handleReset() {
        
        // Send data to the backend via POST
        fetch('http://192.168.2.1:80/control?var=reset&val=1', {  // Enter your IP address here
    
          method: 'GET', 
          mode: 'cors'
    
        })
        
      }
    

    return (
    <>
    <h2>Settings</h2>
    <SettingsCheckBox startUpCheck={startupData.vflip} labelName={"vertical flip"} controlVar={"vflip"}/>
    <SettingsCheckBox startUpCheck={startupData.hmirror} labelName={"Horizontal Mirror"} controlVar={"hmirror"}/>
    <SettingsSlider startUpCheck={startupData.brightness} labelName={"Brightness"} controlVar={"brightness"} minVal={-2} maxVal={2}/>
    <SettingsSlider startUpCheck={startupData.quality} labelName={"Quality"} controlVar={"quality"} minVal={0} maxVal={63}/>
    <SettingsSlider startUpCheck={startupData.contrast} labelName={"Contrast"} controlVar={"contrast"} minVal={-2} maxVal={2}/>
    <SettingsSlider startUpCheck={startupData.saturation} labelName={"Saturation"} controlVar={"saturation"} minVal={-2} maxVal={2}/>
    <SettingsSlider startUpCheck={startupData.sharpness} labelName={"Sharpness"} controlVar={"sharpness"} minVal={-2} maxVal={2}/>
    <SettingsSlider startUpCheck={startupData.framesize} labelName={"Framesize"} controlVar={"framesize"} minVal={0} maxVal={10}/>
    <SettingsSlider startUpCheck={startupData.aec_value} labelName={"Exposure"} controlVar={"aec_value"} minVal={0} maxVal={1200}/>
    <SettingsSlider startUpCheck={startupData.wb_mode} labelName={"White Balance Mode"} controlVar={"wb_mode"} minVal={0} maxVal={4}/>
    <SettingsSlider startUpCheck={startupData.ae_level} labelName={"AE Level"} controlVar={"ae_level"} minVal={-2} maxVal={2}/>
    <SettingsSlider startUpCheck={startupData.agc_gain} labelName={"AGC Gain"} controlVar={"agc_gain"} minVal={0} maxVal={30}/>
    <SettingsSlider startUpCheck={startupData.special_effect} labelName={"Special Effects"} controlVar={"special_effect"} minVal={0} maxVal={6}/>
    <Button onClick={handleSaver} variant="primary">Save</Button>
    <span/>
    <Button onClick={handleReset} variant="primary">Reset</Button>
   </>
    );

}


    export default Settings;