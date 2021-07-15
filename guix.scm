(use-modules (gnu packages autotools)
             (gnu packages radio)
             (guix build-system gnu)
             (guix git-download)
             ((guix licenses) #:prefix license:)
             (guix packages))

(define ofdm-transfer
  (package
    (name "ofdm-transfer")
    (version "1.2.0")
    (source
     (origin
       (method git-fetch)
       (uri (git-reference
             (url "https://github.com/glv2/ofdm-transfer")
             (commit (string-append "v" version))))
       (file-name (git-file-name name version))
       (sha256
        (base32 "1jagvrfzmiajbxmib0833y1s68riwharaf6phrl0112fc9pbdrj1"))))
    (build-system gnu-build-system)
    (native-inputs
     `(("autoconf" ,autoconf)
       ("automake" ,automake)
       ("libtool" ,libtool)))
    (inputs
     `(("liquid-dsp" ,liquid-dsp)
       ("soapysdr" ,soapysdr)))
    (synopsis "Program to transfer data by software defined radio")
    (description
     "@code{ofdm-transfer} is a command-line program to send or receive data by
software defined radio using the OFDM modulation.")
    (home-page "https://github.com/glv2/ofdm-transfer")
    (license license:gpl3+)))

ofdm-transfer
