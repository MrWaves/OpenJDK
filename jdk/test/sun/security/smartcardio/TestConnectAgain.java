/*
 * Copyright 2005 Sun Microsystems, Inc.  All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 USA or visit www.sun.com if you need additional information or
 * have any questions.
 */

/**
 * @test
 * @bug 6239117
 * @summary test connect works correctly if called multiple times/card removed
 * @author Andreas Sterbenz
 * @ignore requires special hardware
 * @run main/manual TestTransmit
 */

import java.io.*;
import java.util.*;

import javax.smartcardio.*;

public class TestConnectAgain extends Utils {

    public static void main(String[] args) throws Exception {
        CardTerminal terminal = getTerminal(args);

        Card card = terminal.connect("T=0");
        CardChannel channel = card.getBasicChannel();
        transmitTestCommand(channel);

        Card card2 = terminal.connect("*");
        if (card != card2) {
            throw new Exception("Different card object");
        }
        card2 = terminal.connect("T=0");
        if (card != card2) {
            throw new Exception("Different card object");
        }

        System.out.println("Remove card!");
        terminal.waitForCardAbsent(0);

        try {
            transmitTestCommand(channel);
            throw new Exception();
        } catch (CardException e) {
            System.out.println("OK: " + e);
        }

        System.out.println("Insert card!");
        terminal.waitForCardPresent(0);

        try {
            transmitTestCommand(channel);
            throw new Exception();
        } catch (IllegalStateException e) {
            System.out.println("OK: " + e);
        }

        card = terminal.connect("*");
        if (card == card2) {
            throw new Exception("Old card object");
        }

        try {
            transmitTestCommand(channel);
            throw new Exception();
        } catch (IllegalStateException e) {
            System.out.println("OK: " + e);
        }

        channel = card.getBasicChannel();
        transmitTestCommand(channel);

        card2 = terminal.connect("*");
        if (card != card2) {
            throw new Exception("Different card object");
        }

        // disconnect
        card.disconnect(false);

        System.out.println("OK.");
    }

}
