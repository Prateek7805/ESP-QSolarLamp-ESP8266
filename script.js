//helper funs

const d = i => document.getElementById(i);
const ae = (o, e, f) => { o.addEventListener(e, f) };

//global variables
//error messages handler
var ermid = null;
const HOST = window.location.origin;
const TIMEOUT_DELAY = 8e3;
//validate credentials
const vdt = (rb)=>{
    const {dn,dp,sn,sp} = rb;
    //device name validation
    if(dn.length<8 || dn.length>20 || dn.includes(' ') || !dn.match(/^[^!#;+\]\/"\t][^+\]"\t]*$/)) return {err: true, msg: "Invalid Device Name"};
    //device password validation
    if(dp.length<8 || dp.length>20 || !dp.match(/^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[!@#$%^&*()_+{}\[\]:;<>,.?~=-])[A-Za-z\d!@#$%^&*()_+{}\[\]:;<>,.?~=-]+$/)) return {err: true, msg: "Invalid Device Password"}
    //STA SSID validation
    if(sn.length === 0 || !sn.match(/^[^!#;+\]\/"\t][^+\]"\t]{0,31}$/)) return {err: true, msg: "Invalid STA SSID"};
    //STA password validation
    if(sp.length === 0 || !sp.match(/^[\u0020-\u007e]{8,63}$/)) return {err: true, msg: "Invalid STA SSID"};
    return {err: false};
}
//display error for 3s
const rerr = ()=>{
    const id = 'ID_ERR_MSG';
    const bid = 'ID_ERR_BG';
    const p_ = d(id);
    const b_ = d(bid);
    p_.innerText = "";
    b_.classList.remove('err_disp');
    ermid = null;
}
const derr = (m) =>{
    const id = 'ID_ERR_MSG';
    const bid = 'ID_ERR_BG';
    const p_ = d(id);
    const b_ = d(bid);
    //add error
    p_.innerText = m;
    if(ermid === null){
       
        b_.classList.add('err_disp');
        ermid = setTimeout(rerr, TIMEOUT_DELAY);
        return;
    }
    clearTimeout(ermid);
    ermid = setTimeout(rerr, TIMEOUT_DELAY);
}

const hBtn = (v) =>{
    const __b = d('ID_SUBMIT');
    __b.disabled = v;
    v? __b.classList.add('btn-disabled'): __b.classList.remove('btn-disabled');
}
//send credentials via POST

const __pf = (req_body) => fetch(`${HOST}/update`, {
    method: 'POST',
    headers: {
        'content-type' : 'application/json'
    },
    body : JSON.stringify(
        req_body
    )
})
.then(res=>{
    hBtn(false);
    if(!res.ok){
        console.log(`HTTP error! Status : ${res.status}`);
        derr(`HTTP error! Status : ${res.status}`);
    }
    return res.text();
}).then(data=>{
    derr(data);
})
.catch(e=>{
    hBtn(false);
    console.log(e);
    derr(e);
})

//submit credentials (e)
const s_creds = () => {
    hBtn(true);
    const dn = d('ID_DEVICE_NAME').value;
    const dp = d('ID_DEVICE_PASS').value;
    const sn = d('ID_STA_SSID').value;
    const sp = d('ID_STA_PASS').value;
    const rb = {
        dn,dp,sn,sp
    }
    const m = vdt(rb);
    if(m.err){
        hBtn(false);
        derr(m.msg);
        return;
    }
    __pf(rb);
}

//update Networks

const _settings = (e) =>{
    fetch(`${HOST}/reset`)
    .catch(e=>derr(e));
    derr("Restarting... to apply the changes");
}
//ready
ae(document, 'DOMContentLoaded', () => {
    ae(d('ID_SUBMIT'), 'click', s_creds);
    ae(d('ID_ALL_RESET'), 'click', _settings);
})
 





