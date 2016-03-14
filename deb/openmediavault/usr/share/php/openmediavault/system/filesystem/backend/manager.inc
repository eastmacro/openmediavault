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
namespace OMV\System\Filesystem\Backend;

require_once("openmediavault/functions.inc");

/**
 * Helper class that implements functions regarding file systems.
 * @ingroup api
 */
class Manager {
	static private $backends = array();

	/**
	 * Register a filesystem backend.
	 * @param backend The filesystem backend object to register.
	 * @return TRUE if successful, otherwise FALSE.
	 */
	final static function registerBackend(BackendAbstract $backend) {
		if (!isset($backend))
			return FALSE;
		self::$backends[] = $backend;
		return TRUE;
	}

	/**
	 * Get the backend of the given filesystem.
	 * @param id The UUID or device path of the filesystem, e.g.
	 *   <ul>
	 *   \li 78b669c1-9183-4ca3-a32c-80a4e2c61e2d (EXT2/3/4, JFS, XFS)
	 *   \li 7A48-BA97 (FAT)
	 *   \li 2ED43920D438EC29 (NTFS)
	 *   \li 2015-01-13-21-48-46-00 (ISO9660)
	 *   \li /dev/sde1
	 *   \li /dev/disk/by-id/scsi-SATA_ST3200XXXX2AS_5XWXXXR6-part1
	 *   \li /dev/disk/by-label/DATA
	 *   \li /dev/disk/by-path/pci-0000:00:10.0-scsi-0:0:0:0-part2
	 *   \li /dev/disk/by-uuid/ad3ee177-777c-4ad3-8353-9562f85c0895
	 *   \li /dev/cciss/c0d0p2
	 *   \li tank/multimedia/movies
	 *   </ul>
	 * @return The backend that implements the given filesystem, otherwise
	 *   NULL.
	 */
	final static function getBackend($id) {
		$result = NULL;
		$enums = self::enumerate();
		foreach ($enums as $enumk => $enumv) {
			$found = FALSE;
			if (is_devicefile($id)) {
				// Compare the device file name. Use the canonical device
				// file for an additional check.
				$found = (($id == $enumv['devicefile']) ||
				  (realpath($id) == $enumv['devicefile']));
			} else if (is_fs_uuid($id)) {
				// Compare the filesystem UUID.
				$found = ($id == $enumv['uuid']);
			} else {
				$found = ($id == $enumv['devicefile']);
			}
			if (TRUE === $found) {
				$result = self::getBackendByType($enumv['type']);
				break;
			}
		}
		/* Slow, but the more correct implementation.
		foreach (self::$backends as $backendk => $backendv) {
			if (FALSE === $backendv->isTypeOf($id))
				continue;
			$result = $backendv;
			break;
		}
		*/
		return $result;
	}

	/**
	 * Get the backend of the given filesystem type.
	 * @param type Specifies the filesystem type, e.g. 'ext3', 'vfat', ...
	 * @return The backend that implements the given filesystem, otherwise
	 *   NULL.
	 */
	final static function getBackendByType($type) {
		$result = NULL;
		foreach (self::$backends as $backendk => $backendv) {
			if (strtolower($type) !== $backendv->getType())
				continue;
			$result = $backendv;
			break;
		}
		return $result;
	}

	/**
	 * Check whether the given file system type is supported.
	 * @param type The file system type, e.g. 'ext4', 'iso9660', ...
	 * @return TRUE if the file system is supported, otherwise FALSE.
	 */
	final static function isSupported($type) {
		if (NULL == ($backend = self::getBackendByType($type)))
			return FALSE;
		return TRUE;
	}

	/**
	 * Get a list of all detected file systems.
	 * @deprecated
	 * @return A list of file systems, otherwise FALSE. The array elements
	 *   contain the fields: \em devicefile, \em uuid, \em label and \em type.
	 *   The devicefile is used as key to access the array elements.
	 */
	public static function get() {
		return $this->enumerate();
	}

	/**
	 * Enumerate all available/detected file systems.
	 * @return A list of file systems, otherwise FALSE. The array elements
	 *   contain the fields: \em devicefile, \em uuid, \em label and \em type.
	 *   The devicefile is used as key to access the array elements.
	 */
	public static function enumerate() {
		// Get all file systems that are identified by the block device
		// identification library.
		// !!! Note !!!
		// File systems that are identified by the block device identification
		// library are processed in one step to increase performance. Otherwise
		// each backend must implement the \ref enumerate method and call the
		// command 'blkid -t TYPE=<xxx> -o full'.
		$cmd = new \OMV\System\Process("blkid", "-o full");
		$cmd->setRedirect2to1(TRUE);
		$cmd->execute($output);
		$result = array();
		foreach ($output as $outputk => $outputv) {
			// Parse command output:
			// /dev/sdb: UUID="e6a0f61e-d969-ffcb-d4c5-06e7711285a3" LABEL="dhcppc4:0" TYPE="linux_raid_member"
			// /dev/sdg1: LABEL="test" UUID="d19bcea0-a323-4aea-9791-c6578180c129" TYPE="jfs"
			// /dev/sda5: UUID="902775c7-8250-4630-81eb-2602ac65e136" TYPE="swap"
			// /dev/sda1: LABEL="Ubuntu" UUID="4aa77ba3-beb6-45dc-b2c7-e77cffd6c6fd" TYPE="ext4"
			// /dev/sdb1: UUID="A27E16CD7E1699DD" TYPE="ntfs"
			// /dev/sdc1: UUID="1a03b79f-d347-4a24-8ae2-62f1aa9a4554" TYPE="ext4"
			// /dev/sdd1: UUID="cb8f7844-362f-4151-8e9e-3888aab723c2" TYPE="ext4"
			// /dev/sda3: UUID="fdca09a8-cbc8-432f-9cba-ce033cb59139" TYPE="ext4" LABEL="Debian"
			// /dev/sr0: LABEL="Debian 7.5.0 amd64 1" TYPE="iso9660"
			// /dev/sr0: UUID="2015-04-25-12-52-13-00" LABEL="Debian 8.0.0 amd64 1" TYPE="iso9660" PTUUID="5e97595e" PTTYPE="dos"
			$regex = '/^(\S+): (.+)$/i';
			if (1 !== preg_match($regex, $outputv, $matches))
				continue;
			// Set default values and extract key/value pairs.
			$data = array(
				"devicefile" => $matches[1],
				"uuid" => "",
				"label" => "",
				"type" => ""
			);
			$parts = preg_split("/(\S+=\\\"[^\\\"]+\\\")|[\s]+/", $matches[2],
			  -1, PREG_SPLIT_NO_EMPTY | PREG_SPLIT_DELIM_CAPTURE);
			foreach ($parts as $partk => $partv) {
				$keyValue = explode("=", $partv);
				if (count($keyValue) != 2)
					continue;
				$data[strtolower($keyValue[0])] = substr($keyValue[1], 1, -1);
			}
			// Is this filesystem processed via blkid?
			if (NULL == ($backend = self::getBackendByType($data['type'])))
				continue;
			if (FALSE === $backend->isBlkidEnumerated())
				continue;
			// Check if there are duplicate file systems. This seems to
			// affect only BTRFS.
			// ToDo: Find a smart way to relocate this check into the
			// file systems \OMV\System\Filesystem\Backend\BackendAbstract
			// implementation.
			if (is_fs_uuid($data['uuid'])) {
				$duplicate = FALSE;
				foreach ($result as $resultk => $resultv) {
					if (fs_uuid_equals($resultv['uuid'], $data['uuid'])) {
						$duplicate = TRUE;
						break;
					}
				}
				// Skip this file system if another one with the same UUID
				// is already processed and added. In this case the first
				// by blkid listed file system wins.
				if (TRUE === $duplicate)
					continue;
			}
			// Append filesystem.
			$result[$data['devicefile']] = $data;
		}
		// Get the file systems that are not identified by the block device
		// identification library.
		foreach (self::$backends as $backendk => $backendv) {
			if (TRUE === $backendv->isBlkidEnumerated())
				continue;
			if (FALSE === ($data = $backendv->enumerate()))
				continue;
			if (empty($data))
				continue;
			$result = array_merge($result, $data);
		}
		return $result;
	}

	/**
	 * Get the object of the class implementing the given filesystem.
	 * @param id The UUID or device path of the filesystem, e.g.
	 *   <ul>
	 *   \li 78b669c1-9183-4ca3-a32c-80a4e2c61e2d (EXT2/3/4, JFS, XFS)
	 *   \li 7A48-BA97 (FAT)
	 *   \li 2ED43920D438EC29 (NTFS)
	 *   \li /dev/sde1
	 *   \li /dev/disk/by-id/scsi-SATA_ST3200XXXX2AS_5XWXXXR6-part1
	 *   \li /dev/disk/by-label/DATA
	 *   \li /dev/disk/by-path/pci-0000:00:10.0-scsi-0:0:0:0-part2
	 *   \li /dev/disk/by-uuid/ad3ee177-777c-4ad3-8353-9562f85c0895
	 *   \li /dev/cciss/c0d0p2
	 *   </ul>
	 * @return The object of the class implementing the given filesystem,
	 *   otherwise NULL.
	 */
	public static function getImpl($id) {
		if (NULL == ($backend = self::getBackend($id)))
			return NULL;
		return $backend->getImpl($id);
	}
}