/*
 * Copyright (c) 2004, 2006, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
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
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package com.sun.corba.se.spi.orbutil.proxy ;

import java.lang.reflect.Method ;
import java.lang.reflect.InvocationHandler ;
import java.lang.reflect.InvocationTargetException ;
import com.sun.corba.se.impl.presentation.rmi.DynamicAccessPermission ;
import com.sun.corba.se.impl.util.Modules;

public abstract class DelegateInvocationHandlerImpl
{
    private DelegateInvocationHandlerImpl() {}

    public static InvocationHandler create( final Object delegate )
    {
        SecurityManager s = System.getSecurityManager();
        if (s != null) {
            s.checkPermission(new DynamicAccessPermission("access"));
        }
        return new InvocationHandler() {
            public Object invoke( Object proxy, Method method, Object[] args )
                throws Throwable
            {
                // This throws an IllegalArgument exception if the delegate
                // is not assignable from method.getDeclaring class.
                try {
                    Modules.ensureReadable(method.getDeclaringClass());
                    return method.invoke( delegate, args ) ;
                } catch (InvocationTargetException ite) {
                    // Propagate the underlying exception as the
                    // result of the invocation
                    throw ite.getCause() ;
                }
            }
        } ;
    }
}