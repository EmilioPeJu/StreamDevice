#!/usr/bin/wish

wm iconify .

proc createTerm {sock} {
    global socket
    toplevel .$sock
    text .$sock.t
    .$sock.t tag configure output -foreground red
    .$sock.t tag configure input -foreground darkgreen
    grid rowconfigure .$sock 0 -weight 1
    grid columnconfigure .$sock 0 -weight 1
    grid .$sock.t -sticky nsew
    bind .$sock.t <Destroy> "close $sock"
    bind .$sock.t <F1> "%W delete 0.1 end"
    set socket(.$sock.t) $sock
    focus .$sock.t
}

socket -server connect 40000

proc connect {sock addr port} {
    fconfigure $sock -blocking 0 -buffering none
    createTerm $sock
    fileevent $sock readable "receiveHandler $sock"
}

proc receiveHandler {sock} {
    set a [read $sock]
    if [eof $sock] {
        destroy .$sock
        return
    }
    .$sock.t insert end $a output
    .$sock.t see end
    if {[string range $a 0 4] == "echo "} {
        .$sock.t insert end [string range $a 5 end]
        .$sock.t see end
        puts -nonewline $sock [string range $a 5 end]
    }
}

if {[info proc tkTextInsert] != ""} {
    set insert tkTextInsert
    set paste tkTextPaste
} else {
    set insert tk::TextInsert
    set paste tk_textPaste
}

rename $insert tkTextInsert_org
rename $paste tkTextPaste_org

proc $insert {w s} {
    global socket
    if {[string equal $s ""] || [string equal [$w cget -state] "disabled"]} {
        return
    }
    puts -nonewline $socket($w) $s
    tkTextInsert_org $w $s
}

proc $paste {w x y} {
    global socket
    set s [selection get -displayof $w]
    puts -nonewline $socket($w) $s
    tkTextPaste_org $w $x $y
    $w see end
}

bind Text <Control-c> [list $insert %W "\x03"]
