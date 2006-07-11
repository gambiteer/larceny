'(load-compiler 'release)
(load "Lib/Common/toplevel.sch")
(let ((arch (assq 'arch-name (system-features))))
  (case (string->symbol (string-downcase (cdr arch)))
    ((sparc) (load "Lib/Sparc/toplevel-target.sch"))
    ((clr)   
     ;(load "Lib/MzScheme/init.sch")
     (load "Lib/IL/toplevel-target.sch"))
    (else 
     (error 'seal-twobit "Add case for " arch))))

; plus some (platform-specific) toplevel-target file

(define toplevel-macro-expand #f)       ; A hack for the benefit of 
                                        ; init-toplevel-environment
                                        ; FSK: this seems like a bug 
                                        ; workaround to me...
                                        ; its conspiring with def'n
                                        ; of macro-expand below
(define toplevel-macro-expand macro-expand)

(define (seal-twobit proc-names)
  ;; Install twobit's macro expander as the interpreter's ditto
  ;; FSK: I'm not too thrilled about this either.
  (macro-expander (lambda (form environment)
                    (let ((switches (compiler-switches 'get)))
		      (dynamic-wind
			  (lambda ()
			    (compiler-switches 'standard))
			  (lambda ()
			    (twobit-expand form (environment-syntax-environment environment)))
			  (lambda ()
			    (compiler-switches 'set! switches))))))
  
  (let ((e (interaction-environment)))
    (letrec ((install-procedures
              (lambda (x procs)
                (if (not (null? procs))
                    (begin
                      (environment-set! x
                                        (car procs)
                                        (environment-get e (car procs)))
                      (install-procedures x (cdr procs)))))))
      (init-toplevel-environment)
      (install-procedures (interaction-environment) proc-names)))
  (eval 
   '(define macro-expand 
      (lambda (expr . rest)
        (let ((env (if (null? rest)
                       (interaction-environment)
                       (car rest))))
          (macro-expand-expression expr env))))
   (interaction-environment))
  (unspecified)
  )
  
(define standard-proc-names  
  '(; Compilation
    compile
    assemble
    compile-file
    assemble-file
    compile-expression
    macro-expand-expression
    process-file
    assembly-declarations
    dump-fasl-segment-to-port
                                        ; On-line help
    help
                                        ; Compiler and assembler switches
    compiler-switches
    compiler-flags
    global-optimization-flags
    runtime-safety-flags
    issue-warnings
    include-procedure-names
    include-source-code
    include-variable-names
    single-stepping
    avoid-space-leaks
    runtime-safety-checking
    catch-undefined-globals
    integrate-procedures
    control-optimization
    parallel-assignment-optimization
    lambda-optimization
    benchmark-mode
    benchmark-block-mode
    global-optimization
    interprocedural-inlining
    interprocedural-constant-propagation
    common-subexpression-elimination
    representation-inference
    local-optimization
    peephole-optimization
    inline-allocation
                                        ; Make utility
    make:project
    make:new-project
    make:project?
    make:rule
    make:deps
    make:targets
    make:pretend
    make:debug
    make:make))