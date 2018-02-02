/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

jsonData = "data.json";

function load() {
	console.log('Basecamp loaded');
	var request = new XMLHttpRequest();
	function transferComplete() {
			        var data = JSON.parse(this.responseText);
				buildSite(data.elements);
		}; 
	function transferFailed(evt) {
			console.log('Looks like there was a problem. Status Code: ' + request.status);
			addElementToDom("h1","error","Could not load configuration: " + request.status, {"style": "color:red"});
			return;
		};
	request.addEventListener("load", transferComplete);
	request.addEventListener("error", transferFailed);
	request.open("GET", jsonData);
	request.send();

};

function configureElement(element) {
	switch(element.element)  {
		case "input":

			if (element.content) {
				labelId = "labelfor" + element.id;
				labelAttr = { "for": element.id };
				console.log(element.content);
				addElementToDom("label", labelId , element.content, labelAttr, element.parent);
				addElementToDom("input", element.id, "", element.attributes, "#"+labelId);
			} else {
				addElementToDom(element.element, element.id, element.content, element.attributes, element.parent);
			}
			break;
		default:
			addElementToDom(element.element, element.id, element.content, element.attributes, element.parent);
			break;
	}
}

function addElementToDom (elementType, elementID, elementContent) { 
	var elementAttributes = arguments.length > 3 && arguments[3] !== undefined ? arguments[3] : {};
	var parentSelector = arguments[4];
	var newElement = document.createElement(elementType); 
	if (elementContent) {
		var newContent = document.createTextNode(elementContent); 
		newElement.appendChild(newContent);
	}
	if (elementID) newElement.setAttribute("id", elementID);

	setAttributes(newElement, elementAttributes);
	var parent = document.querySelector(parentSelector);
	if(!parent) parent = document.querySelector("#wrapper");
	parent.appendChild(newElement);
}

function setAttributes(el, attrs) {
	for(var key in attrs) {
		if (attrs[key]) {
			el.setAttribute(key, attrs[key]);
		}
	}
}

function buildSite (data) {
	var selectedParent = "#wrapper";
	console.log(data);
	while (data.length > 0) {
		var elementGroup = []

		for(var i = 0;i < data.length; i++)  {
			if (data[i].parent == selectedParent) {	
				elementGroup.push(data[i]);
				data.splice(i,1);
				break;
			}
		}
		for(var i = 0;i < elementGroup.length; i++)
		{
			configureElement(elementGroup[i]);
		}
		if (data.length > 0) {
			selectedParent = data[0].parent;
		}
	}

}

function collectConfiguration() {
	document.getElementById("WifiConfigured").value="True";
	var configurationData = new FormData();
	configurationElements = document.body.querySelectorAll('*[data-config]');
	for (var i = configurationElements.length - 1; i >= 0; i--) {
		var configurationKey = configurationElements[i].getAttribute("data-config");
		var configurationValue = configurationElements[i].value;
		if (configurationElements[i].hasAttribute("required") && configurationValue == "") {
			alert("Please fill out all required values");
			return;	
		}
		if (configurationValue.length > 0 && configurationKey.length > 0) { 
			configurationData.append(configurationKey, configurationValue);
		}
	}
	var request = new XMLHttpRequest();
	request.addEventListener("load", transferComplete);
	request.addEventListener("error", transferFailed);
	request.open("POST", "/submitconfig");
	console.log("Sending configuration");
	request.send(configurationData);
	function transferFailed(){
		alert("Configuration could not be saved");
	}
	function transferComplete(){
		alert("Configuration saved successfully. Rebooting.");
	};

}

window.onload = function() {
	load();
}
