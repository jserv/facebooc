/**/
(function($) {
  // Closes the Responsive Menu on Menu Item Click
  $('.navbar-collapse ul li a').click(function() {
      $('.navbar-toggle:visible').click();
  });

  angular.element(document).ready(function(){
    angular.bootstrap(document.body, ['mainApp']);
  });

  window.main_app = window.main_app || angular.module('mainApp', [ 'ngAnimate' ], function($interpolateProvider) {
    $interpolateProvider.startSymbol('[[');
    $interpolateProvider.endSymbol(']]');
  });

  window.main_app.value('GLOBAL_VALUES',{
    EMAIL : 'service@facebooc.com',
    TITLE : 'Facebooc',
    BROWSER_CHECKER: {
      // Opera 8.0+
      isOpera: ( (!!window.opr && !!opr.addons) || !!window.opera || navigator.userAgent.indexOf(' OPR/') >= 0 ),
      // Firefox 1.0+
      isFirefox: ( typeof InstallTrigger !== 'undefined' ),
      // At least Safari 3+: "[object HTMLElementConstructor]"
      isSafari: ( Object.prototype.toString.call(window.HTMLElement).indexOf('Constructor') > 0 ),
      // Internet Explorer 6-11
      isIE: /*@cc_on!@*/false || !!document.documentMode,
      // Edge 20+
      isEdge: ( !(false || !!document.documentMode) && !!window.StyleMedia ),
      // Chrome 1+
      isChrome: ( !!window.chrome && !!window.chrome.webstore ),
      // Blink engine detection
      isBlink: ( ( ( !!window.chrome && !!window.chrome.webstore ) || ( (!!window.opr && !!opr.addons) || !!window.opera || navigator.userAgent.indexOf(' OPR/') >= 0 ) ) && !!window.CSS )
    }
  });
})(jQuery); // End of use strict