/*
   Basecamp - ESP32 library to simplify the basics of IoT projects
   Written by Merlin Schumacher (mls@ct.de) for c't magazin für computer technik (https://www.ct.de)
   Licensed under GPLv3. See LICENSE for details.
   */

jsonData = "data.json";

function load() {
	console.log('Basecamp loaded');
	fetch(jsonData)
		.then(
			function(response) {
				if (response.status !== 200) {
					console.log('Looks like there was a problem. Status Code: ' +
						response.status);
					addElementToDom("h1","error","Could not load configuration: "+response.status, {"style": "color:red"});
					return;
				}

				response.json().then(function(data) {
					buildSite(data.elements);
					setMeta(data.meta);
				});
			}
		)
		.catch(function(err) {
			console.log('Fetch Error :-S', err);
			addElementToDom("h1","error","Could not load configuration", {"style": "color:red"});
		});

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

function addElementToDom (elementType, elementID, elementContent, elementAttributes = {}, parentSelector) { 
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

	for(var i = 0;i < data.length; i++)
	{
		configureElement(data[i]);
	}

}

function setMeta(data) {
	document.title = data.title;
}

function collectConfiguration() {
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
			configurationData.set(configurationKey, configurationValue);
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
