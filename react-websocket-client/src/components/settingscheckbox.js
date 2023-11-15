import React, { useState, useEffect, useRef } from 'react';
import Form from 'react-bootstrap/Form';
import Row from 'react-bootstrap/Row';

function SettingsCheckBox({startUpCheck, labelName,controlVar}) {
    const [ value, setValue ] = useState(0); 

    function getVal(checking) {
      return checking ? `1` : `0`;
    }

    return (
      <>
    <Row>
    <Form>
      <Form.Check // prettier-ignore
        type="switch"
        id="custom-switch"
        label={labelName}
        defaultChecked = {startUpCheck}
        onClick={(e) => {
          console.log(e.target.checked);
          fetch(`http://192.168.2.1:80/control?var=${controlVar}`+`&val=${getVal(e.target.checked)}`, {  // Enter your IP address here
          method: 'GET', 
          mode: 'cors'
        })
        }}
      />
    </Form>
    </Row>
    <Row/>
    </>
    );

}


    export default SettingsCheckBox;