var mConfig = {};
var id;
//var locationOptions = {"enableHighAccuracy": true, "timeout":60000, "maximumAge": 0};
var locationOptions = {"timeout":60000, "maximumAge": 0};
var intLongitude;
var intLatitude;
var transactionID;

Pebble.addEventListener("ready", function(e) {
	console.log("luna is ready");
  //id = navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
  id = navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  //loadLocalData();
  //returnConfigToPebble();
});

Pebble.addEventListener("appmessage", function(e) {
  navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  //console.log(e.type);
  //console.log("message!");
});

Pebble.addEventListener("showConfiguration", function(e) {
	Pebble.openURL(mConfig.configureUrl);
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      returnConfigToPebble();
    }
  }
);

function saveLocalData(config) {
  console.log("saveLocalData() " + JSON.stringify(config));
  localStorage.setItem("TZ1Name", config.TZ1Name);
  localStorage.setItem("TZ1", parseInt(config.TZ1));  
  localStorage.setItem("TZ2Name", config.TZ2Name);
  localStorage.setItem("TZ2", parseInt(config.TZ2));  
  localStorage.setItem("LATITUDE", parseInt(config.LATITUDE));	
  localStorage.setItem("LONGITUDE", parseInt(config.LONGITUDE));	
  localStorage.setItem("TZSS", parseInt(config.TZSS));
  localStorage.setItem("invert", parseInt(config.invert)); 
  localStorage.setItem("dmy", parseInt(config.dmy)); 
  localStorage.setItem("lang", parseInt(config.lang)); 
  loadLocalData();
}

function loadLocalData() {
	mConfig.TZ1Name =  localStorage.getItem("TZ1Name");
	mConfig.TZ1 = parseInt(localStorage.getItem("TZ1"));
	mConfig.TZ2Name =  localStorage.getItem("TZ2Name");
	mConfig.TZ2 = parseInt(localStorage.getItem("TZ2"));
	mConfig.LATITUDE = localStorage.getItem("LATITUDE");
	mConfig.LONGITUDE = localStorage.getItem("LONGITUDE");
	mConfig.TZSS = parseInt(localStorage.getItem("TZSS"));
	mConfig.invert = parseInt(localStorage.getItem("invert"));
	mConfig.dmy = parseInt(localStorage.getItem("dmy"));
	mConfig.lang = parseInt(localStorage.getItem("lang"));
	mConfig.configureUrl = "http://goo.gl/fou7kz";
	//mConfig.configureUrl = "http://192.168.1.200/90hank/index2.html";
	
	if(isNaN(mConfig.TZ1)) {
		mConfig.TZ1 = 0;
	}
	if(isNaN(mConfig.TZ2)) {
		mConfig.TZ2 = 0;
	}
	if(isNaN(mConfig.TZSS)) {
		mConfig.TZSS = 0;
	}	
	if(isNaN(mConfig.LATITUDE)) {
		mConfig.LATITUDE = 0;
	}	
	if(isNaN(mConfig.LONGITUDE)) {
		mConfig.LONGITUDE = 0;
	}		
	if(isNaN(mConfig.invert)) {
		mConfig.invert = 0;
	}
	if(isNaN(mConfig.dmy)) {
		mConfig.dmy = 0;
	}
	if(isNaN(mConfig.lang)) {
		mConfig.lang = 0;
	}
	console.log("loadLocalData() " + JSON.stringify(mConfig));
}

function returnConfigToPebble() {
 console.log("Configuration window returned: " + JSON.stringify(mConfig));
  Pebble.sendAppMessage({
	"TZ1Name": mConfig.TZ1Name,
    "TZ1":parseInt(mConfig.TZ1), 
	"TZ2Name": mConfig.TZ2Name,
    "TZ2":parseInt(mConfig.TZ2), 
	"TZSS":parseInt(mConfig.TZSS),  
    "LATITUDE":parseInt(mConfig.LATITUDE), 
    "LONGITUDE":parseInt(mConfig.LONGITUDE),   
    "invert":parseInt(mConfig.invert),
	"dmy":parseInt(mConfig.dmy),
	"lang":parseInt(mConfig.lang)
  });    
}





function locationSuccess(pos){
    var coordinates = pos.coords;
    var changed = 0;
    if (intLongitude != Math.round( coordinates.longitude )) {
          intLongitude = Math.round ( coordinates.longitude);
          changed = 1;
        }
    if (intLatitude != Math.round( coordinates.latitude )) {
          intLatitude = Math.round ( coordinates.latitude );
          changed = 1;
        }
    if (Boolean(changed)) {
      transactionID = Pebble.sendAppMessage({ 
        "KEY_LONGITUDE": coordinates.longitude,
        "KEY_LATITUDE" : coordinates.latitude
      });
    }
}

function locationError(err){
    console.log("Error getting location.");
}