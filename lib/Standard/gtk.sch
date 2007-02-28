(require 'std-ffi)
(require 'foreign-stdlib)
(require 'foreign-sugar)
(require 'glib) ;; convenience; who's going to use gtk without glib?
(require 'gdk)

(let ((os (assq 'os-name (system-features))))
  (cond 
   ((equal? os '(os-name . "Linux"))
    (foreign-file "/usr/lib/libgtk-x11-2.0.so.0"))    
   ((equal? os '(os-name . "SunOS"))
    (foreign-file "/usr/lib/libgtk-x11-2.0.so"))
   ((equal? os '(os-name . "MacOS X"))
    (foreign-file "/sw/lib/libgtk-x11-2.0.dylib"))
   (else
    (error "Add case in gtk.sch for os: " os))))

;;; XXX perhaps we need a bit of type abstraction for callbacks
;;; like these, where we could abstract over what type should be
;;; ascribed to the callback argument.
;;;
;;; (But that's an absurd amount of machinery to add when one can
;;; easily get around the problem by passing closures as the callback,
;;; as would have been appropriate in the first place)

(establish-void*-subhierarchy!
 '(gobject* (gtkobject* 
             (gtkwidget* (gtkcontainer* 
                          (gtkbin* (gtkalignment*)
                                   (gtkwindow* (gtkdialog*))
                                   (gtkbutton* (gtktogglebutton*
                                                (gtkcheckbutton*
                                                 (gtkradiobutton*)))
                                               (gtkoptionmenu*))
                                   (gtkcombobox*)
                                   (gtktoolitem* (gtktoolbutton*))
                                   (gtkframe*)
                                   (gtkitem* (gtkmenuitem*
                                              (gtkcheckmenuitem*
                                               (gtkradiomenuitem*))
                                              (gtkimagemenuitem*)
                                              (gtkseparatormenuitem*)
                                              (gtktearoffmenuitem*))))
                          (gtkbox* (gtkvbox*)
                                   (gtkhbox*
                                    (gtkcombo*)))
                          (gtktable*)
                          (gtktoolbar*)
                          (gtkmenushell* (gtkmenubar*)
                                         (gtkmenu*)))
                         (gtkmisc* (gtklabel*)
                                   (gtkarrow*)
                                   (gtkimage*)
                                   (gtkpixmap*))
                         (gtkrange* (gtkscale*)
                                    (gtkscrollbar*))
                         (gtkprogress* (gtkprogressbar*)))
             (gtktooltips*)
             (gtkadjustment*))
            (gtkaccelgroup*)))

(define gtk-init 
  (let ()
    (define-foreign (gtk-init void* void*) void)
    (lambda arg-strings
      (let ((string-vec (list->vector arg-strings)))
        (call-with-char** string-vec
                          (lambda (argv)
                            (call-with-boxed 
                             argv 
                             (lambda (&argv)
                               (call-with-boxed (vector-length string-vec)
                                                (lambda (&argc)
                                                  (gtk-init &argc &argv)))))))))))

(define-foreign (gtk-window-new int) gtkwindow*)
(define-foreign (gtk-widget-show gtkwidget*) void)
(define-foreign (gtk-widget-add-accelerator gtkwidget* string gtkaccelgroup* 
                                            uint uint uint) 
  void)
(define-foreign (gtk-widget-get-parent gtkwidget*) gtkwidget*)
(define-foreign (gtk-widget-set-parent gtkwidget* gtkwidget*) void)
(define-foreign (gtk-widget-get-parent-window gtkwidget*) gdkwindow*)
(define-foreign (gtk-widget-set-parent-window gtkwidget* gdkwindow*) void)

(define-foreign (gtk-main) void)
(define-foreign (gtk-main-quit) void)

(define-foreign (gtk-container-set-border-width! gtkcontainer* int) void)
(define-foreign (gtk-button-new-with-label string) gtkbutton*)
(define-foreign (gtk-container-add gtkcontainer* gtkwidget*) void)
(define-foreign (gtk-window-set-title! gtkwindow* string) void)
(define-foreign (gtk-window-set-resizable! gtkwindow* bool) void)
(define-foreign (gtk-hbox-new bool int) gtkhbox*)
(define-foreign (gtk-vbox-new bool int) gtkvbox*)
(define-foreign (gtk-label-new string) gtkwidget*)
(define-foreign (gtk-hseparator-new) gtkwidget*)
(define-foreign (gtk-box-pack-start gtkbox* gtkwidget* bool bool int) void)
(define-foreign (gtk-box-pack-end   gtkbox* gtkwidget* bool bool int) void)
(define-foreign (gtk-misc-set-alignment gtkmisc* int int) void)
(define-foreign (gtk-widget-set-size-request gtkwidget* int int) void)
(define-foreign (gtk-table-new int int bool) gtkwidget*)
(define-foreign (gtk-table-attach-defaults gtktable* gtkwidget* int int int int)
  void)
(define-foreign (gtk-image-new-from-file string) gtkwidget*)
(define-foreign (gtk-button-new) gtkwidget*)
(define-foreign (gtk-radio-button-new-with-label (maybe void*) string) 
  gtkradiobutton*)
(define-foreign (gtk-radio-button-new-with-label-from-widget 
                 gtkradiobutton* string) 
  gtkradiobutton*)
(define-foreign (gtk-radio-button-get-group gtkradiobutton*) void*)
(define-foreign (gtk-toggle-button-set-active gtktogglebutton* bool) void)
(define-foreign (gtk-toggle-button-get-active gtktogglebutton*) bool)
(define-foreign (gtk-adjustment-new double double double double double double) gtkobject*)
(define-foreign (gtk-vscale-new gtkadjustment*) gtkwidget*)
(define-foreign (gtk-hscale-new gtkadjustment*) gtkwidget*)
(define-foreign (gtk-hscrollbar-new gtkadjustment*) gtkwidget*)
(define-foreign (gtk-check-button-new-with-label string) gtkwidget*)
(define-foreign (gtk-range-set-update-policy gtkrange* int) void)
(define-foreign (gtk-scale-set-digits gtkscale* int) void)
(define-foreign (gtk-scale-set-value-pos gtkscale* int) void)
(define-foreign (gtk-scale-set-draw-value gtkscale* bool) void)
(define-foreign (gtk-menu-item-new-with-label string) gtkmenuitem*)
(define-foreign (gtk-menu-item-new) gtkmenuitem*)
(define-foreign (gtk-menu-new) gtkmenu*)
(define-foreign (gtk-option-menu-new) gtkoptionmenu*)
(define-foreign (gtk-menu-shell-append gtkmenushell* gtkwidget*) void)
(define-foreign (gtk-option-menu-set-menu gtkoptionmenu* gtkwidget*) void)
(define-foreign (gtk-frame-new string) gtkframe*)
(define-foreign (gtk-label-set-justify gtklabel* int) void)
(define-foreign (gtk-label-set-line-wrap gtklabel* bool) void)
(define-foreign (gtk-label-set-pattern gtklabel* string) void)
(define-foreign (gtk-widget-show-all gtkwidget*) void)
(define-foreign (gtk-arrow-new int int) gtkwidget*)
(define-foreign (gtk-alignment-new float float float float) gtkalignment*)
(define-foreign (gtk-progress-bar-new) gtkprogressbar*)
(define-foreign (gtk-table-attach gtktable* gtkwidget* 
                                  uint uint uint uint 
                                  unsigned unsigned 
                                  uint uint) 
  void)
(define-foreign (gtk-widget-destroy gtkwidget*) void)
(define-foreign (gtk-progress-bar-get-fraction gtkprogressbar*) double)
(define-foreign (gtk-progress-bar-set-fraction gtkprogressbar* double) void)
(define-foreign (gtk-progress-bar-get-text gtkprogressbar*) string)
(define-foreign (gtk-progress-bar-set-text gtkprogressbar* string) void)
(define-foreign (gtk-progress-bar-pulse gtkprogressbar*) void)
(define-foreign (gtk-progress-bar-get-orientation gtkprogressbar*) unsigned)
(define-foreign (gtk-progress-bar-set-orientation gtkprogressbar* unsigned) void)
(define-foreign (gtk-container-set-border-width gtkcontainer* uint) void)
(define-foreign (gtk-container-get-border-width gtkcontainer*) uint)


(define-foreign (gtk-tooltips-new) gtktooltips*)
(define-foreign (gtk-tooltips-set-tip gtktooltips* gtkwidget* string (maybe string)) void)

(define-foreign (gtk-menu-bar-new) gtkmenubar*)

(define-foreign (gtk-accel-group-new) gtkaccelgroup*)
;; (define-foreign (gtk-accel-group-attach void* void*) void) ;; XXX

(define-foreign (gtk-menu-item-set-submenu gtkmenuitem* gtkwidget*) void)
(define-foreign (gtk-menu-item-get-submenu gtkmenuitem*) gtkwidget*)

(define-foreign (gtk-check-menu-item-new) gtkcheckmenuitem*)
(define-foreign (gtk-check-menu-item-new-with-label string) gtkcheckmenuitem*)
(define-foreign (gtk-check-menu-item-new-with-mnemonic string) gtkcheckmenuitem*)
(define-foreign (gtk-check-menu-item-set-active gtkcheckmenuitem* bool) void)
(define-foreign (gtk-check-menu-item-get-active gtkcheckmenuitem*) bool)
(define-foreign (gtk-check-menu-item-toggled gtkcheckmenuitem*) void)
(define-foreign (gtk-check-menu-item-set-inconsistent gtkcheckmenuitem* bool) void)
(define-foreign (gtk-check-menu-item-get-inconsistent gtkcheckmenuitem*) bool)

(define-foreign (gtk-radio-menu-item-new (maybe void*)) gtkradiomenuitem*)
(define-foreign (gtk-radio-menu-item-new-with-label (maybe void*) string) gtkradiomenuitem*)
(define-foreign (gtk-radio-menu-item-new-with-mnemonic (maybe void*) string) gtkradiomenuitem*)
(define-foreign (gtk-radio-menu-item-get-group gtkradiomenuitem*) void*)
(define-foreign (gtk-radio-menu-item-set-group gtkradiomenuitem* (maybe void*)) void)

(define-foreign (gtk-toolbar-get-type) uint)
(define-foreign (gtk-toolbar-new) gtktoolbar*)
(define-foreign (gtk-toolbar-get-orientation gtktoolbar*) int)
(define-foreign (gtk-toolbar-set-orientation gtktoolbar* int) void)
(define-foreign (gtk-toolbar-get-tooltips gtktoolbar*) bool)
(define-foreign (gtk-toolbar-set-tooltips gtktoolbar* bool) void)
(define-foreign (gtk-toolbar-get-style gtktoolbar*) int)
(define-foreign (gtk-toolbar-set-style gtktoolbar* int) void)
(define-foreign (gtk-toolbar-append-item 
                 gtktoolbar* string string string gtkwidget* 
                 (-> (gtkwidget* void*) void) (maybe void*))
  void)
(define-foreign (gtk-toolbar-append-space gtktoolbar*) void)
(define-foreign (gtk-toolbar-append-widget gtktoolbar* gtkwidget* string string) void)
(define-foreign (gtk-toolbar-prepend-widget gtktoolbar* gtkwidget* string string) void)
(define-foreign (gtk-toolbar-insert-widget gtktoolbar* gtkwidget* string string int) void)
(define-foreign (gtk-toolbar-append-element 
                 gtktoolbar* uint (maybe gtkwidget*) 
                 (maybe string) string (maybe string) 
                 gtkwidget* (-> (gtkwidget* void*) void) (maybe void*)) 
  gtkwidget*)
(define-foreign (gtk-toolbar-prepend-element 
                 gtktoolbar* uint (maybe gtkwidget*) 
                 (maybe string) string (maybe string) 
                 gtkwidget* (-> (gtkwidget* void*) void) (maybe void*)) 
  void*)
(define-foreign (gtk-toolbar-prepend-element 
                 gtktoolbar* uint (maybe gtkwidget*)
                 (maybe string) string (maybe string) 
                 gtkwidget* (-> (gtkwidget* void*) void) (maybe void*) int) 
  gtkwidget*)

(define-foreign (gtk-pixmap-new gdkpixmap* (maybe gdkbitmap*)) gtkpixmap*)

(define-foreign (gtk-combo-new) gtkcombo*)
(define-foreign (gtk-combo-set-value-in-list gtkcombo* bool bool) void)
(define-foreign (gtk-combo-set-use-arrows gtkcombo* bool) void)
(define-foreign (gtk-combo-set-use-arrows-always gtkcombo* bool) void)
(define-foreign (gtk-combo-set-case-sensitive gtkcombo* bool) void)
(define-foreign (gtk-combo-set-item-string gtkcombo* gtkitem* string) void)
(define-foreign (gtk-combo-set-popdown-strings gtkcombo* glist*) void)
(define-foreign (gtk-combo-disable-activate gtkcombo*) void)

(define GTK-WINDOW-TOPLEVEL 0)

(define GTK-EXPAND 1)
(define GTK-SHRINK 2)
(define GTK-FILL   4)

(define GTK-UPDATE-CONTINUOUS 0)
(define GTK-UPDATE-DISCONTINUOUS 1)
(define GTK-UPDATE-DELAYED 0)

(define GTK-POS-LEFT 0)
(define GTK-POS-RIGHT 1)
(define GTK-POS-TOP 2)
(define GTK-POS-BOTTOM 3)

(define GTK-JUSTIFY-LEFT 0)
(define GTK-JUSTIFY-RIGHT 1)
(define GTK-JUSTIFY-CENTER 2)
(define GTK-JUSTIFY-FILL 3)

(define GTK-ARROW-UP 0)
(define GTK-ARROW-DOWN 1)
(define GTK-ARROW-LEFT 2)
(define GTK-ARROW-RIGHT 3)

(define GTK-SHADOW-NONE 0)
(define GTK-SHADOW-IN 1)
(define GTK-SHADOW-OUT 2)
(define GTK-SHADOW-ETCHED-IN 3)
(define GTK-SHADOW-ETCHED-OUT 4)

(define GTK-PROGRESS-LEFT-TO-RIGHT 0)
(define GTK-PROGRESS-RIGHT-TO-LEFT 1)
(define GTK-PROGRESS-BOTTOM-TO-TOP 2)
(define GTK-PROGRESS-TOP-TO-BOTTOM 3)

(define GTK-ACCEL-VISIBLE #b01)
(define GTK-ACCEL-LOCKED  #b10)
(define GTK-ACCEL-MASK    #x07)

(define GTK-ORIENTATION-HORIZONTAL 0)
(define GTK-ORIENTATION-VERTICAL 1)

(define GTK-TOOLBAR-ICONS 0)
(define GTK-TOOLBAR-TEXT  1)
(define GTK-TOOLBAR-BOTH  2)
(define GTK-TOOLBAR-BOTH-HORIZ 3)

;; BELOW ARE DEPRECATED ACCORDING TO GTK+ HEADER FILES...
(define GTK-TOOLBAR-CHILD-SPACE 0)
(define GTK-TOOLBAR-CHILD-BUTTON 1)
(define GTK-TOOLBAR-CHILD-TOGGLEBUTTON 2)
(define GTK-TOOLBAR-CHILD-RADIOBUTTON 3)
(define GTK-TOOLBAR-CHILD-WIDGET 4)
