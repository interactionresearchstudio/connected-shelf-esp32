import React, { useState, useEffect, useRef } from 'react';
import {
    Dropdown,
    ButtonGroup,
    Row,
    SplitButton,
    Form
  } from "react-bootstrap";

function SettingsDropdown({labelName,controlVar,startUpCheck}) {
    const [currentVal, setCurrentVal] = useState("");
    const [currentLabel, setCurrentLabel] = useState("");
    const onClick = (value) => {
        setCurrentVal(value);
        console.log(value);
        if(value === 1){     
            setCurrentLabel("1600x1000");
            fetch(`http://192.168.2.1:80/control?var=${controlVar}`+`&val=14`, {  // Enter your IP address here
            method: 'GET', 
            mode: 'cors'
            })
        }else if(value === 2){
            setCurrentLabel("1600x1200");
            fetch(`http://192.168.2.1:80/control?var=${controlVar}`+`&val=13`, {  // Enter your IP address here
            method: 'GET', 
            mode: 'cors'
            })
        } else if(value === 3){
            setCurrentLabel("1280x720");
            fetch(`http://192.168.2.1:80/control?var=${controlVar}`+`&val=8`, {  // Enter your IP address here
            method: 'GET', 
            mode: 'cors'
            })
        } else if(value === 4){
            setCurrentLabel("640x480");
            fetch(`http://192.168.2.1:80/control?var=${controlVar}`+`&val=8`, {  // Enter your IP address here
            method: 'GET', 
            mode: 'cors'
            })
        }
    }
    return (
<Row>
<Form.Label>
          {labelName}
</Form.Label>
<Dropdown as={ButtonGroup}>
{/* <DropdownToggle menuAlign="left" split /> */}
{/* <Dropdown.Menu> */}
<SplitButton
  menuAlign={{ lg: "left" }}
  title={currentVal === "" ? labelName : currentLabel}
>
  <Dropdown.Item onClick={() => onClick(1)} eventKey={1}>
    1600x1000 (Default)
  </Dropdown.Item>
  <Dropdown.Item onClick={() => onClick(2)} eventKey={2}>
    1600x1200
  </Dropdown.Item>
  <Dropdown.Item onClick={() => onClick(3)} eventKey={3}>
    1280x720
  </Dropdown.Item>
  <Dropdown.Item onClick={() => onClick(4)} eventKey={4}>
    640x480
  </Dropdown.Item>    
</SplitButton>
{/* </Dropdown.Menu> */}
</Dropdown>
</Row>
);
}


    export default SettingsDropdown;