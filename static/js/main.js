(function($) {
  /* ng section */
  // boostarp ng app
  angular.element(document).ready(function(){
    angular.bootstrap(document.body, ['mainApp']);
  });

  // create app and reset interpolation
  window.main_app = window.main_app || angular.module('mainApp', [ 'ngAnimate' ], function($interpolateProvider) {
    $interpolateProvider.startSymbol('[[');
    $interpolateProvider.endSymbol(']]');
  });

  // set values for whole app
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

  /* controllers */
  // navbar Controller
  var navbarController = function($rootScope, $scope, $http, $window, $location, GLOBAL_VALUES){
    var ctrl = this;

    // play midi with timbre
    ctrl.play_beatbox = function(){
      /* timbre.js (http://mohayonao.github.io/timbre.js/beatbox.html) */
      T("audio").load("/static/timbre/audio/drumkit.wav", function() {
        var BD  = this.slice(   0,  500).set({bang:false});
        var SD  = this.slice( 500, 1000).set({bang:false});
        var HH1 = this.slice(1000, 1500).set({bang:false, mul:0.2});
        var HH2 = this.slice(1500, 2000).set({bang:false, mul:0.2});
        var CYM = this.slice(2000).set({bang:false, mul:0.2});
        var scale = new sc.Scale([0,1,3,7,8], 12, "Pelog");

        var P1 = [
          [BD, HH1],
          [HH1],
          [HH2],
          [],
          [BD, SD, HH1],
          [HH1],
          [HH2],
          [SD],
        ].wrapExtend(128);

        var P2 = sc.series(16);
        var drum = T("lowshelf", {freq:110, gain:8, mul:0.6}, BD, SD, HH1, HH2, CYM).play();
        var lead = T("saw", {freq:T("param")});
        var vcf  = T("MoogFF", {freq:2400, gain:6, mul:0.1}, lead);
        var env  = T("perc", {r:100});
        var arp  = T("OscGen", {wave:"sin(15)", env:env, mul:0.5});

        T("delay", {time:"BPM128 L4", fb:0.65, mix:0.35}, 
          T("pan", {pos:0.2}, vcf), 
          T("pan", {pos:T("tri", {freq:"BPM64 L1", mul:0.8}).kr()}, arp)
        ).play();

        T("interval", {interval:"BPM128 L16"}, function(count) {
          var i = count % P1.length;
          if (i === 0) CYM.bang();

          P1[i].forEach(function(p) { p.bang(); });

          if (Math.random() < 0.015) {
            var j = (Math.random() * P1.length)|0;
            P1.wrapSwap(i, j);
            P2.wrapSwap(i, j);
          }

          var noteNum = scale.wrapAt(P2.wrapAt(count)) + 60;
          if (i % 2 === 0) {
            lead.freq.linTo(noteNum.midicps() * 2, "100ms");
          }
          arp.noteOn(noteNum + 24, 60);
        }).start();
      });
    }

    ctrl.init_navbar = function(){
      ctrl.play_beatbox();
    }
  }
  navbarController.$injector = ['$rootScope', '$scope', '$http', '$window', '$location', 'GLOBAL_VALUES'];
  window.main_app.controller('navbarCtrl', navbarController);

  // index Controller
  var indexController = function($rootScope, $scope, $http, $window, $location, GLOBAL_VALUES){
    var ctrl = this;
    ctrl.init_index = function(){
      console.log('Hello Facebooc Index Page');
    }
  }
  indexController.$injector = ['$rootScope', '$scope', '$http', '$window', '$location', 'GLOBAL_VALUES'];
  window.main_app.controller('indexCtrl', indexController);

})(jQuery);
