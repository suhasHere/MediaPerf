/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/* This code is loaded in every child process that is started by mochitest in
 * order to be used as a replacement for UniversalXPConnect
 */

var Ci = Components.interfaces;
var Cc = Components.classes;


function SpecialPowersAPI() { 
  this._mfl = null;
}

function bindDOMWindowUtils(aWindow) {
  if (!aWindow)
    return

  var util = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                   .getInterface(Components.interfaces.nsIDOMWindowUtils);
  // This bit of magic brought to you by the letters
  // B Z, and E, S and the number 5.
  //
  // Take all of the properties on the nsIDOMWindowUtils-implementing
  // object, and rebind them onto a new object with a stub that uses
  // apply to call them from this privileged scope. This way we don't
  // have to explicitly stub out new methods that appear on
  // nsIDOMWindowUtils.
  //
  // Note that this will be a chrome object that is (possibly) exposed to
  // content. Make sure to define __exposedProps__ for each property to make
  // sure that it gets through the security membrane.
  var proto = Object.getPrototypeOf(util);
  var target = { __exposedProps__: {} };
  function rebind(desc, prop) {
    if (prop in desc && typeof(desc[prop]) == "function") {
      var oldval = desc[prop];
      try {
        desc[prop] = function() {
          return oldval.apply(util, arguments);
        };
      } catch (ex) {
        dump("WARNING: Special Powers failed to rebind function: " + desc + "::" + prop + "\n");
      }
    }
  }
  for (var i in proto) {
    var desc = Object.getOwnPropertyDescriptor(proto, i);
    rebind(desc, "get");
    rebind(desc, "set");
    rebind(desc, "value");
    Object.defineProperty(target, i, desc);
    target.__exposedProps__[i] = 'rw';
  }
  return target;
}

function isWrappable(x) {
  if (typeof x === "object")
    return x !== null;
  return typeof x === "function";
};

function isWrapper(x) {
  return isWrappable(x) && (typeof x.SpecialPowers_wrappedObject !== "undefined");
};

function unwrapIfWrapped(x) {
  return isWrapper(x) ? unwrapPrivileged(x) : x;
};

function wrapIfUnwrapped(x) {
  return isWrapper(x) ? x : wrapPrivileged(x);
}

function callGetOwnPropertyDescriptor(obj, name) {
  // Quickstubbed getters and setters are propertyOps, and don't get reified
  // until someone calls __lookupGetter__ or __lookupSetter__ on them (note
  // that there are special version of those functions for quickstubs, so
  // apply()ing Object.prototype.__lookupGetter__ isn't good enough). Try to
  // trigger reification before calling Object.getOwnPropertyDescriptor.
  //
  // See bug 764315.
  try {
    obj.__lookupGetter__(name);
    obj.__lookupSetter__(name);
  } catch(e) { }
  return Object.getOwnPropertyDescriptor(obj, name);
}

// We can't call apply() directy on Xray-wrapped functions, so we have to be
// clever.
function doApply(fun, invocant, args) {
  return Function.prototype.apply.call(fun, invocant, args);
}

function wrapPrivileged(obj) {

  // Primitives pass straight through.
  if (!isWrappable(obj))
    return obj;

  // No double wrapping.
  if (isWrapper(obj))
    throw "Trying to double-wrap object!";

  // Make our core wrapper object.
  var handler = new SpecialPowersHandler(obj);

  // If the object is callable, make a function proxy.
  if (typeof obj === "function") {
    var callTrap = function() {
      // The invocant and arguments may or may not be wrappers. Unwrap them if necessary.
      var invocant = unwrapIfWrapped(this);
      var unwrappedArgs = Array.prototype.slice.call(arguments).map(unwrapIfWrapped);

      return wrapPrivileged(doApply(obj, invocant, unwrappedArgs));
    };
    var constructTrap = function() {
      // The arguments may or may not be wrappers. Unwrap them if necessary.
      var unwrappedArgs = Array.prototype.slice.call(arguments).map(unwrapIfWrapped);

      // Constructors are tricky, because we can't easily call apply on them.
      // As a workaround, we create a wrapper constructor with the same
      // |prototype| property. ES semantics dictate that the return value from
      // |new| is the return value of the |new|-ed function i.f.f. the returned
      // value is an object. We can thus mimic the behavior of |new|-ing the
      // underlying constructor just be passing along its return value in our
      // constructor.
      var FakeConstructor = function() {
        return doApply(obj, this, unwrappedArgs);
      };
      FakeConstructor.prototype = obj.prototype;

      return wrapPrivileged(new FakeConstructor());
    };

    return Proxy.createFunction(handler, callTrap, constructTrap);
  }

  // Otherwise, just make a regular object proxy.
  return Proxy.create(handler);
};

function unwrapPrivileged(x) {

  // We don't wrap primitives, so sometimes we have a primitive where we'd
  // expect to have a wrapper. The proxy pretends to be the type that it's
  // emulating, so we can just as easily check isWrappable() on a proxy as
  // we can on an unwrapped object.
  if (!isWrappable(x))
    return x;

  // If we have a wrappable type, make sure it's wrapped.
  if (!isWrapper(x))
    throw "Trying to unwrap a non-wrapped object!";

  // Unwrap.
  return x.SpecialPowers_wrappedObject;
};

function crawlProtoChain(obj, fn) {
  alert(" In Crawl");
  var rv = fn(obj);
  if (rv !== undefined)
    return rv;
  if (Object.getPrototypeOf(obj))
    return crawlProtoChain(Object.getPrototypeOf(obj), fn);
};

/*
 * We want to waive the __exposedProps__ security check for SpecialPowers-wrapped
 * objects. We do this by creating a proxy singleton that just always returns 'rw'
 * for any property name.
 */
function ExposedPropsWaiverHandler() {
  alert("In ExposedPropsWaiverHandler");
  // NB: XPConnect denies access if the relevant member of __exposedProps__ is not
  // enumerable.
  var _permit = { value: 'rw', writable: false, configurable: false, enumerable: true };
  return {
    getOwnPropertyDescriptor: function(name) { return _permit; },
    getPropertyDescriptor: function(name) { return _permit; },
    getOwnPropertyNames: function() { throw Error("Can't enumerate ExposedPropsWaiver"); },
    getPropertyNames: function() { throw Error("Can't enumerate ExposedPropsWaiver"); },
    enumerate: function() { throw Error("Can't enumerate ExposedPropsWaiver"); },
    defineProperty: function(name) { throw Error("Can't define props on ExposedPropsWaiver"); },
    delete: function(name) { throw Error("Can't delete props from ExposedPropsWaiver"); }
  };
};
ExposedPropsWaiver = Proxy.create(ExposedPropsWaiverHandler());

function SpecialPowersHandler(obj) {
  this.wrappedObject = obj;
};

// Allow us to transitively maintain the membrane by wrapping descriptors
// we return.
SpecialPowersHandler.prototype.doGetPropertyDescriptor = function(name, own) {

  // Handle our special API.
  if (name == "SpecialPowers_wrappedObject")
    return { value: this.wrappedObject, writeable: false, configurable: false, enumerable: false };

  // Handle __exposedProps__.
  if (name == "__exposedProps__")
    return { value: ExposedPropsWaiver, writable: false, configurable: false, enumerable: false };

  // In general, we want Xray wrappers for content DOM objects, because waiving
  // Xray gives us Xray waiver wrappers that clamp the principal when we cross
  // compartment boundaries. However, Xray adds some gunk to toString(), which
  // has the potential to confuse consumers that aren't expecting Xray wrappers.
  // Since toString() is a non-privileged method that returns only strings, we
  // can just waive Xray for that case.
  var obj = name == 'toString' ? XPCNativeWrapper.unwrap(this.wrappedObject)
                               : this.wrappedObject;

  //
  // Call through to the wrapped object.
  //
  // Note that we have several cases here, each of which requires special handling.
  //
  var desc;

  if (own) 
    desc = callGetOwnPropertyDescriptor(obj, name);

  // Case 2: Not own, not Xray-wrapped.
  //
  // Here, we can just crawl the prototype chain, calling
  // Object.getOwnPropertyDescriptor until we find what we want.
  //
  // NB: Make sure to check this.wrappedObject here, rather than obj, because
  // we may have waived Xray on obj above.
  //else if (!isXrayWrapper(this.wrappedObject))
   //else 
  //  desc = crawlProtoChain(obj, function(o) {return callGetOwnPropertyDescriptor(o, name);});

  // Case 3: Not own, Xray-wrapped.
  //
  // This one is harder, because we Xray wrappers are flattened and don't have
  // a prototype. Xray wrappers are proxies themselves, so we'd love to just call
  // through to XrayWrapper<Base>::getPropertyDescriptor(). Unfortunately though,
  // we don't have any way to do that. :-(
  //
  // So we first try with a call to getOwnPropertyDescriptor(). If that fails,
  // we make up a descriptor, using some assumptions about what kinds of things
  // tend to live on the prototypes of Xray-wrapped objects.
  else {
    desc = Object.getOwnPropertyDescriptor(obj, name);
    if (!desc) {
      var getter = Object.prototype.__lookupGetter__.call(obj, name);
      var setter = Object.prototype.__lookupSetter__.call(obj, name);
      if (getter || setter)
        desc = {get: getter, set: setter, configurable: true, enumerable: true};
      else if (name in obj)
        desc = {value: obj[name], writable: false, configurable: true, enumerable: true};
    }
  }

  // Bail if we've got nothing.
  if (typeof desc === 'undefined')
    return undefined;

  // When accessors are implemented as JSPropertyOps rather than JSNatives (ie,
  // QuickStubs), the js engine does the wrong thing and treats it as a value
  // descriptor rather than an accessor descriptor. Jorendorff suggested this
  // little hack to work around it. See bug 520882.
  if (desc && 'value' in desc && desc.value === undefined)
    desc.value = obj[name];

  // A trapping proxy's properties must always be configurable, but sometimes
  // this we get non-configurable properties from Object.getOwnPropertyDescriptor().
  // Tell a white lie.
  desc.configurable = true;

  // Transitively maintain the wrapper membrane.
  function wrapIfExists(key) { if (key in desc) desc[key] = wrapPrivileged(desc[key]); };
  wrapIfExists('value');
  wrapIfExists('get');
  wrapIfExists('set');

  return desc;
};

SpecialPowersHandler.prototype.getOwnPropertyDescriptor = function(name) {
  return this.doGetPropertyDescriptor(name, true);
};

SpecialPowersHandler.prototype.getPropertyDescriptor = function(name) {
  return this.doGetPropertyDescriptor(name, false);
};

function doGetOwnPropertyNames(obj, props) {

  // Insert our special API. It's not enumerable, but getPropertyNames()
  // includes non-enumerable properties.
  var specialAPI = 'SpecialPowers_wrappedObject';
  if (props.indexOf(specialAPI) == -1)
    props.push(specialAPI);

  // Do the normal thing.
  var flt = function(a) { return props.indexOf(a) == -1; };
  props = props.concat(Object.getOwnPropertyNames(obj).filter(flt));

  // If we've got an Xray wrapper, include the expandos as well.
  if ('wrappedJSObject' in obj)
    props = props.concat(Object.getOwnPropertyNames(obj.wrappedJSObject)
                         .filter(flt));

  return props;
}

SpecialPowersHandler.prototype.getOwnPropertyNames = function() {
  return doGetOwnPropertyNames(this.wrappedObject, []);
};

SpecialPowersHandler.prototype.getPropertyNames = function() {

  // Manually walk the prototype chain, making sure to add only property names
  // that haven't been overridden.
  //
  // There's some trickiness here with Xray wrappers. Xray wrappers don't have
  // a prototype, so we need to unwrap them if we want to get all of the names
  // with Object.getOwnPropertyNames(). But we don't really want to unwrap the
  // base object, because that will include expandos that are inaccessible via
  // our implementation of get{,Own}PropertyDescriptor(). So we unwrap just
  // before accessing the prototype. This ensures that we get Xray vision on
  // the base object, and no Xray vision for the rest of the way up.
  var obj = this.wrappedObject;
  var props = [];
  while (obj) {
    props = doGetOwnPropertyNames(obj, props);
    obj = Object.getPrototypeOf(XPCNativeWrapper.unwrap(obj));
  }
  return props;
};

SpecialPowersHandler.prototype.defineProperty = function(name, desc) {
  return Object.defineProperty(this.wrappedObject, name, desc);
};

SpecialPowersHandler.prototype.delete = function(name) {
  return delete this.wrappedObject[name];
};

SpecialPowersHandler.prototype.fix = function() { return undefined; /* Throws a TypeError. */ };

// Per the ES5 spec this is a derived trap, but it's fundamental in spidermonkey
// for some reason. See bug 665198.
SpecialPowersHandler.prototype.enumerate = function() {
  var t = this;
  var filt = function(name) { return t.getPropertyDescriptor(name).enumerable; };
  return this.getPropertyNames().filter(filt);
};

function wrapCallback(cb) {
  return function SpecialPowersCallbackWrapper() {
    args = Array.prototype.map.call(arguments, wrapIfUnwrapped);
    return cb.apply(this, args);
  }
}

function wrapCallbackObject(obj) {
  wrapper = { __exposedProps__: ExposedPropsWaiver };
  for (var i in obj) {
    if (typeof obj[i] == 'function')
      wrapper[i] = wrapCallback(obj[i]);
    else
      wrapper[i] = obj[i];
  }
  return wrapper;
}

SpecialPowersAPI.prototype = {
  wrap: wrapIfUnwrapped,
  unwrap: unwrapIfWrapped,
  isWrapper: isWrapper,
  wrapCallback: wrapCallback,
  wrapCallbackObject: wrapCallbackObject,
  // Mimic the get*Pref API
  getBoolPref: function(aPrefName) {
    return (this._getPref(aPrefName, 'BOOL'));
  },
  getIntPref: function(aPrefName) {
    return (this._getPref(aPrefName, 'INT'));
  },
  getCharPref: function(aPrefName) {
    return (this._getPref(aPrefName, 'CHAR'));
  },
  getComplexValue: function(aPrefName, aIid) {
    return (this._getPref(aPrefName, 'COMPLEX', aIid));
  },

  // Mimic the set*Pref API
  setBoolPref: function(aPrefName, aValue) {
    return (this._setPref(aPrefName, 'BOOL', aValue));
  },
  setIntPref: function(aPrefName, aValue) {
    return (this._setPref(aPrefName, 'INT', aValue));
  },
  setCharPref: function(aPrefName, aValue) {
    return (this._setPref(aPrefName, 'CHAR', aValue));
  },
  setComplexValue: function(aPrefName, aIid, aValue) {
    return (this._setPref(aPrefName, 'COMPLEX', aValue, aIid));
  },

  // Mimic the clearUserPref API
  clearUserPref: function(aPrefName) {
    var msg = {'op':'clear', 'prefName': aPrefName, 'prefType': ""};
    this._sendSyncMessage('SPPrefService', msg);
  },

  // Private pref functions to communicate to chrome
  _getPref: function(aPrefName, aPrefType, aIid) {
    var msg = {};
    if (aIid) {
      // Overloading prefValue to handle complex prefs
      msg = {'op':'get', 'prefName': aPrefName, 'prefType':aPrefType, 'prefValue':[aIid]};
    } else {
      msg = {'op':'get', 'prefName': aPrefName,'prefType': aPrefType};
    }
    var val = this._sendSyncMessage('SPPrefService', msg);

    if (val == null || val[0] == null)
      throw "Error getting pref";
    return val[0];
  },
  _setPref: function(aPrefName, aPrefType, aValue, aIid) {
    var msg = {};
    if (aIid) {
      msg = {'op':'set','prefName':aPrefName, 'prefType': aPrefType, 'prefValue': [aIid,aValue]};
    } else {
      msg = {'op':'set', 'prefName': aPrefName, 'prefType': aPrefType, 'prefValue': aValue};
    }
    return(this._sendSyncMessage('SPPrefService', msg)[0]);
  },

  getConsoleMessages: function() {
    var consoleService = Cc['@mozilla.org/consoleservice;1'].getService(Ci.nsIConsoleService);
    var messages = {}
    consoleService.getMessageArray(messages, {});
    var retVal = {}
    retVal.value = []
    for (var i = 0; i < messages.value.length; i++) {
      ival = messages.value[i];
      if (ival === undefined || ival == null)
        continue;

      retVal.value[i] = {}
      rval = retVal.value[i];
      for (var obj in ival) {
        rval[obj] = ival[obj];
      }
    }
    return retVal;
  },

  get ID() {
    var appInfo = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
    try {
      var id = appInfo.ID;
      return id;
    } catch(err) {};
    return null;
  },

  get Version() {
    var appInfo = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
    return appInfo.version;
  },

  get BuildID() {
    var appInfo = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
    return appInfo.appBuildID;
  },

  isAccessible: function() {
    return Cc["@mozilla.org/accessibleRetrieval;1"].getService(Ci.nsIAccessibleRetrieval);
  },

  getAccessible: function(aAccOrElmOrID, aInterfaces)
  {
    if (!aAccOrElmOrID) {
      return null;
    }

    var elm = null;

    if (aAccOrElmOrID instanceof Ci.nsIAccessible) {
      aAccOrElmOrID.QueryInterface(Ci.nsIAccessNode);
      elm = aAccOrElmOrID.DOMNode;
    } else if (aAccOrElmOrID instanceof Ci.nsIDOMNode) {
      elm = aAccOrElmOrID;
    } else {
      elm = this.window.get().document.getElementById(aAccOrElmOrID);
    }

    var acc = (aAccOrElmOrID instanceof Ci.nsIAccessible) ? aAccOrElmOrID : null;
    if (!acc) {
      try {
        acc = gAccRetrieval.getAccessibleFor(elm);
      } catch (e) {
      }
    }

    if (!aInterfaces) {
      return acc;
    }

    if (aInterfaces instanceof Array) {
      for (var index = 0; index < aInterfaces.length; index++) {
        try {
          acc.QueryInterface(aInterfaces[index]);
        } catch (e) {
        }
      }
      return acc;
    }
  
    try {
      acc.QueryInterface(aInterfaces);
    } catch (e) {
    }
  
    return acc;
  },

  setLogFile: function(path) {
    this._mfl = new MozillaFileLogger(path);
  },

  log: function(data) {
    if (this._mfl) {
      this._mfl.log(data);
    }
  },

  test: function() {
    alert("In Test");
  },

  closeLogFile: function() {
    if (this._mfl) {
      this._mfl.close();
    }
  },

};
