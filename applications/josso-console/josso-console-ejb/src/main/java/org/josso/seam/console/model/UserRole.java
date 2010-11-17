/*
 * JOSSO: Java Open Single Sign-On
 *
 * Copyright 2004-2009, Atricore, Inc.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 *
 */
package org.josso.seam.console.model;

import org.hibernate.validator.NotNull;

import javax.persistence.*;

/**
 * UserRole generated by hbm2java
 */
@Entity
@Table(name = "JOSSO_USER_ROLE", catalog = "PUBLIC")
public class UserRole implements java.io.Serializable {

    private static final long serialVersionUID = 1L;
    
	private Integer id;
	private Username username;
	private Role role;

	public UserRole() {
	}

	public UserRole(Username username, Role role) {
		this.username = username;
		this.role = role;
	}

	@Id
	@GeneratedValue(strategy = GenerationType.TABLE)
	@Column(name = "ID", unique = true, nullable = false)
	public Integer getId() {
		return this.id;
	}

	public void setId(Integer id) {
		this.id = id;
	}
	@ManyToOne(fetch = FetchType.LAZY)
	@JoinColumn(name = "LOGIN", nullable = false)
	@NotNull
	public Username getUsername() {
		return this.username;
	}

	public void setUsername(Username username) {
		this.username = username;
	}
	@ManyToOne(fetch = FetchType.LAZY)
	@JoinColumn(name = "ROLE", nullable = false)
	@NotNull
	public Role getRole() {
		return this.role;
	}

	public void setRole(Role role) {
		this.role = role;
	}

}