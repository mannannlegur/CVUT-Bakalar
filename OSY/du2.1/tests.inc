/* SW RAID5 - basic test
 *
 * The testing of the RAID driver requires a backend (simulating the underlying disks).
 * Next, the tests of your RAID implemetnation are needed. To help you with the implementation,
 * a sample backend is implemented in this file. It provides a quick-and-dirty
 * implementation of the underlying disks (simulated in files) and a few Raid... function calls.
 *
 * The implementation in the real testing environment is different. The sample below is a
 * minimalistic disk backend which matches the required interface. The backend, for instance,
 * cannot simulate a crashed disk. To test your Raid implementation, you will have to modify
 * or extend the backend.
 *
 * Next, you will have to add some raid testing. There is a few Raid... functions called from within
 * main(), however, the tests are incomplete. For instance, RaidResync () is not tested here. Once
 * again, this is only a starting point.
 */

#define RAID_DEVICES 4
#define DISK_SECTORS 8192

static FILE  * g_Fp[RAID_DEVICES];
static bool flag = false;
static bool flag1 = true;

//-------------------------------------------------------------------------------------------------
/** Sample sector reading function. The function will be called by your Raid driver implementation.
 * Notice, the function is not called directly. Instead, the function will be invoked indirectly
 * through function pointer in the TBlkDev structure.
 */
int                diskRead                                ( int               device,
                                                             int               sectorNr,
                                                             void            * data,
                                                             int               sectorCnt )
{
  if ( device < 0 || device >= RAID_DEVICES )
    return 0;
  if ( g_Fp[device] == NULL )
    return 0;
  if ( sectorCnt <= 0 || sectorNr + sectorCnt > DISK_SECTORS )
    return 0;
  fseek ( g_Fp[device], sectorNr * SECTOR_SIZE, SEEK_SET );
  return fread ( data, SECTOR_SIZE, sectorCnt, g_Fp[device] );
}
//-------------------------------------------------------------------------------------------------
/** Sample sector writing function. Similar to diskRead
 */
int                diskWrite                               ( int               device,
                                                             int               sectorNr,
                                                             const void      * data,
                                                             int               sectorCnt )
{
  if (device == 1 && ! flag) return 0;
  if (device == 2 && ! flag1) return 0;

  if ( device < 0 || device >= RAID_DEVICES )
    return 0;
  if ( g_Fp[device] == NULL )
    return 0;
  if ( sectorCnt <= 0 || sectorNr + sectorCnt > DISK_SECTORS )
    return 0;
  fseek ( g_Fp[device], sectorNr * SECTOR_SIZE, SEEK_SET );
  return fwrite ( data, SECTOR_SIZE, sectorCnt, g_Fp[device] );
}
//-------------------------------------------------------------------------------------------------
/** A function which releases resources allocated by openDisks/createDisks
 */
void               doneDisks                               ( TBlkDev         * dev )
{
  delete dev;
  for ( int i = 0; i < RAID_DEVICES; i ++ )
    if ( g_Fp[i] )
    {
      fclose ( g_Fp[i] );
      g_Fp[i]  = NULL;
    }
}
//-------------------------------------------------------------------------------------------------
/** A function which creates the files needed for the sector reading/writing functions above.
 * This function is only needed for the particular implementation above.
 */
TBlkDev          * createDisks                             ( void )
{
  char       buffer[SECTOR_SIZE];
  TBlkDev  * res = NULL;
  char       fn[100];

  memset    ( buffer, 0, sizeof ( buffer ) );

  for ( int i = 0; i < RAID_DEVICES; i ++ )
  {
    snprintf ( fn, sizeof ( fn ), "/tmp/%04d", i );
    g_Fp[i] = fopen ( fn, "w+b" );

    if ( ! g_Fp[i] )
    {
      doneDisks ( NULL );
      return NULL;
    }

    for ( int j = 0; j < DISK_SECTORS; j ++ )
      if ( fwrite ( buffer, sizeof ( buffer ), 1, g_Fp[i] ) != 1 )
      {
        doneDisks ( NULL );
        return NULL;
      }
  }

  res              = new TBlkDev;
  res -> m_Devices = RAID_DEVICES;
  res -> m_Sectors = DISK_SECTORS;
  res -> m_Read    = diskRead;
  res -> m_Write   = diskWrite;
  return res;
}
//-------------------------------------------------------------------------------------------------
/** A function which opens the files needed for the sector reading/writing functions above.
 * This function is only needed for the particular implementation above.
 */
TBlkDev          * openDisks                               ( void )
{
  TBlkDev  * res = NULL;
  char       fn[100];

  for ( int i = 0; i < RAID_DEVICES; i ++ )
  {
    snprintf ( fn, sizeof ( fn ), "/tmp/%04d", i );
    g_Fp[i] = fopen ( fn, "r+b" );
    if ( ! g_Fp[i] )
    {
      doneDisks ( NULL );
      return NULL;
    }
    fseek ( g_Fp[i], 0, SEEK_END );
    if ( ftell ( g_Fp[i] ) != DISK_SECTORS * SECTOR_SIZE )
    {
      doneDisks ( NULL );
      return NULL;
    }
  }

  res              = new TBlkDev;
  res -> m_Devices = RAID_DEVICES;
  res -> m_Sectors = DISK_SECTORS;
  res -> m_Read    = diskRead;
  res -> m_Write   = diskWrite;
  return res;
}
//-------------------------------------------------------------------------------------------------
int main ( void )
{
  /* create the disks before we use them
   */
  TBlkDev * dev = createDisks ();
  /* The disks are ready at this moment. Your RAID-related functions may be executed,
   * the disk backend is ready.
   *
   * First, try to create the RAID:
   */

  int retCode = RaidCreate ( dev );
  assert ( retCode );

  /* start it */

  RaidStart ( dev );

  /* your raid device shall be up.
   * try to read and write all RAID sectors:
   */

  unsigned char buffer[SECTOR_SIZE];
  memset(buffer, 0xCC, SECTOR_SIZE);

  assert(RaidWrite(0, buffer, 1) == 1);

  /* Extensive testing of your RAID implementation ...
   */


  /* Stop the raid device ...
   */
  RaidStop ();

  /* ... and the underlying disks.
   */

  doneDisks ( dev );

  /* The RAID as well as disks are stopped. It corresponds i.e. to the
   * restart of a real computer.
   *
   * after the restart, we will not create the disks, nor create RAID (we do not
   * want to destroy the content). Instead, we will only open/start the devices:
   */

  dev = openDisks ();
  RaidStart ( dev );

  assert(RaidResync() == 0);
  assert(RaidStatus() == RAID_DEGRADED);
  memset(buffer, 0xDD, SECTOR_SIZE);
  assert(RaidWrite(0, buffer, 1) == 1);

  RaidStop ();
  doneDisks ( dev );

  flag = true;

  dev = openDisks ();
  RaidStart ( dev );

  assert(RaidResync() == 1);
  assert(RaidStatus() == RAID_OK);
  memset(buffer, 0xEE, SECTOR_SIZE);
  assert(RaidWrite(0, buffer, 1) == 1);

  RaidStop ();
  doneDisks ( dev );

  return 0;
}