import React, { useState, useEffect, useRef } from 'react';
import RangeSlider from 'react-bootstrap-range-slider';
import Form from 'react-bootstrap/Form';
import Row from 'react-bootstrap/Row';
import Col from 'react-bootstrap/Col';


function SettingsSlider({startUpCheck, labelName,controlVar,minVal,maxVal}) {
    const [ value, setValue ] = useState(0); 
    const [ finalValue, setFinalValue ] = useState(startUpCheck);
    useEffect(() => {
      setValue(startUpCheck);
    }, [startUpCheck])

    return (
      <Form>
      <Form.Group as={Row}>
      <Form.Label>
          {labelName}
        </Form.Label>
        <Col sm="8">
        <RangeSlider
          min={minVal}
          max={maxVal}
          value={value}
          onChange={changeEvent => setValue(changeEvent.target.value)}
          onAfterChange={(e) => {
            console.log(e.target.value);
            setFinalValue(e.target.value);
            fetch(`http://192.168.2.1:80/control?var=${controlVar}`+`&val=${e.target.value}`, {  // Enter your IP address here
            method: 'GET', 
            mode: 'cors'
          })
          }}/>
        </Col>
      </Form.Group>
      <Row/>
      </Form>
    );

}


    export default SettingsSlider;