let socket = new WebSocket("ws://192.168.3.14:8000");

socket.onopen = (e) => {
    alert("Established Connection to RasPi");
}

socket.onerror = (e) => {
    console.log("Error in Connection:");
    cnsolge.log(e);
}

function send_data() {
    let step_delay = document.getElementById("step_delay").value;
    let base_deg = document.getElementById("base_deg").value;
    let shoulder_deg = document.getElementById("shoulder_deg").value;
    let elbow_deg = document.getElementById("elbow_deg").value;
    let vert_wrist_deg = document.getElementById("vert_wrist_deg").value;
    let rot_wrist_deg = document.getElementById("rot_wrist_deg").value;
    let gripper = document.getElementById("gripper").value;

    var command = "m("
    command += pad(step_delay, 2) + ",";
    command += pad(base_deg, 3) + ",";
    command += pad(shoulder_deg, 3) + ",";
    command += pad(elbow_deg, 3) + ",";
    command += pad(vert_wrist_deg, 3) + ",";
    command += pad(rot_wrist_deg, 3) + ",";
    command += pad(gripper, 2) + ")";

    socket.send(command);
}

function start_position() {
    socket.send("neutral");
}

function pad(num, size) {
    num = num.toString();
    while (num.length < size) {
        num = "0" + num;
    } 
    return num;
}