<?php
/**
 * This file is part of OpenMediaVault.
 *
 * @license   http://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2016 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenMediaVault. If not, see <http://www.gnu.org/licenses/>.
 */
namespace OMV\Config;

require_once("openmediavault/globals.inc");
require_once("openmediavault/functions.inc");
require_once("openmediavault/object.inc");
require_once("openmediavault/error.inc");
require_once("openmediavault/config.inc");
require_once("openmediavault/config/configobject.inc");

class Database extends \OMVObject {
	/**
	 * Returns the configuration database singleton.
	 * @return The singleton object.
	 */
	public static function &getInstance() {
		static $instance = NULL;
		if (!isset($instance))
			$instance = new Database();
		return $instance;
	}

	private function validateObject($object) {
		if (FALSE === ($object instanceof ConfigObject)) {
			$this->setLastError("Incorrect type of object attribute.");
			return FALSE;
		}
		if (FALSE === $object->isInitialized()) {
			$this->setLastError("The object attribute is not initialized.");
			return FALSE;
		}
		return TRUE;
	}

	/**
	 * Get the configuration at the given XPath expression.
	 * @param xpath The XPath expression to execute.
	 * @param id The data model identifier. If \em id is not set, then
	 *   the xpath will be used as identifier instead.
	 * @return The configuration object, otherwise FALSE.
	 */
	public function get($xpath, $id = "") {
		global $xmlConfig;
		if (empty($id))
			$id = $xpath;
		$data = $xmlConfig->get($xpath);
		if (is_null($data)) {
			$this->setLastError("XPath expression '%s' does not match.",
			  $xpath);
			return FALSE;
		}
		$object = new ConfigObject($id);
		if (FALSE === $object->initialize()) {
			$this->setLastError($object->getLastError());
			return FALSE;
		}
		if (!$object->setAssoc($data, FALSE)) {
			$this->setLastError($object->getLastError());
			return FALSE;
		}
		return $object;
	}

	/**
	 * Set the configuration at the given XPath expression.
	 * @param xpath The XPath expression to execute.
	 * @param object The configuration object to use.
	 * @return TRUE if successful, otherwise FALSE.
	 */
	public function set($xpath, $object) {
		global $xmlConfig;
		if (FALSE === $this->validateObject())
			return FALSE;
		$data = $object->getAssoc();
		if (FALSE === $xmlConfig->set($xpath, $data)) {
			$this->setLastError("XPath expression '%s' does not match.",
			  $xpath);
			return FALSE;
		}
		return TRUE;
	}

	/**
	 * Replace the configuration at the given XPath expression.
	 * @param xpath The XPath expression to execute.
	 * @param object The configuration object to use.
	 * @return TRUE if successful, otherwise FALSE.
	 */
	public function replace($xpath, $object) {
		global $xmlConfig;
		if (FALSE === $this->validateObject())
			return FALSE;
		$data = $object->getAssoc();
		if (FALSE === $xmlConfig->replace($xpath, $data)) {
			$this->setLastError("XPath expression '%s' does not match.",
			  $xpath);
			return FALSE;
		}
		return TRUE;
	}

	/**
	 * Update the existing configuration at the given XPath expression.
	 * The given data will override the existing configuration. If the
	 * given XPath expression does not exist, then the method will exit
	 * immediatelly.
	 * @param xpath The XPath expression to execute.
	 * @param object The configuration object to use.
	 * @return TRUE if successful, otherwise FALSE.
	 */
	public function update($xpath, $object) {
		global $xmlConfig;
		if (FALSE === $this->validateObject())
			return FALSE;
		$data = $object->getAssoc();
		if (FALSE === $xmlConfig->update($xpath, $data)) {
			$this->setLastError("XPath expression '%s' does not match.",
			  $xpath);
			return FALSE;
		}
		return TRUE;
	}

	/**
	 * Delete the nodes matching the given XPath expression.
	 * @param xpath The XPath expression to execute.
	 * @return Returns the deleted nodes, otherwise FALSE.
	 */
	public function delete($xpath) {
		if (FALSE === $xmlConfig->delete($xpath)) {
			$this->setLastError("XPath expression '%s' does not match.",
			  $xpath);
			return FALSE;
		}
		return TRUE;
	}
}